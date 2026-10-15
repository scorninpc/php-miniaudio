#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "zend_exceptions.h"
#include "php_miniaudio.h"

#define MINIAUDIO_IMPLEMENTATION
#include "deps/miniaudio.h"

zend_class_entry *miniaudio_ce;
zend_object_handlers miniaudio_object_handlers;

// variaveis estaticas globais do miniaudio
static ma_engine global_audio_engine;
static bool global_engine_initialized = false;

// estrutura individual do estado da engine miniaudio
typedef struct {
	ma_sound sound;					// audio carregado

	bool sound_initialized;			// inicializado?
	bool is_playing;				// esta todando?

	zend_object std;				// objeto zend

} php_miniaudio_object;

// helper que recupera o objeto c de um zend_objeto
static inline php_miniaudio_object *php_miniaudio_fetch_object(zend_object *obj) {
	return (php_miniaudio_object *)((char *)obj - XtOffsetOf(php_miniaudio_object, std));
}

#define Z_MINIAUDIO_P(zv) php_miniaudio_fetch_object(Z_OBJ_P(zv))

// destrutor do objeto PHP
static void miniaudio_object_free(zend_object *object) {
	php_miniaudio_object *intern = php_miniaudio_fetch_object(object);

	if (intern->sound_initialized) {
		ma_sound_stop(&intern->sound);
		ma_sound_uninit(&intern->sound);
	}

	zend_object_std_dtor(&intern->std);
}

// aloca o objeto php no zend engine
static zend_object *miniaudio_object_create(zend_class_entry *ce) {
	php_miniaudio_object *intern = (php_miniaudio_object *)emalloc(sizeof(php_miniaudio_object) + zend_object_properties_size(ce));
	
	intern->sound_initialized = false;
	intern->is_playing = false;

	zend_object_std_init(&intern->std, ce);
	object_properties_init(&intern->std, ce);
	intern->std.handlers = &miniaudio_object_handlers;

	return &intern->std;
}

/**
 * callback para setar o is_playing como false
 */
void onfinish_callback(void* pUserData, ma_sound* pSound) {

	// recupera o objeto
	php_miniaudio_object *intern = (php_miniaudio_object *)pUserData;
	if (intern) {
		// informa a finalização
		intern->is_playing = false;
	}
}

/**
 * construtor, ja deve receber o arquivo de audio, pois cada objeto só toca um audio
 */
PHP_METHOD(MiniAudio, __construct)
{
	char *filepath;
	size_t filepath_len;

	// verifica o parametro
	if(zend_parse_parameters(ZEND_NUM_ARGS(), "s", &filepath, &filepath_len) == FAILURE) {
		return;
	}

	// verifica se a engine ta inicializada
	if(!global_engine_initialized) {
		zend_throw_exception(zend_ce_exception, "Audio engine global nao inicializada", 0);
		RETURN_THROWS();
	}

	// recupera o objeto 
	php_miniaudio_object *intern = Z_MINIAUDIO_P(getThis());

	// carrega o audio
	if(ma_sound_init_from_file(&global_audio_engine, filepath, 0, NULL, NULL, &intern->sound) != MA_SUCCESS) {
		zend_throw_exception(zend_ce_exception, "Falha ao carregar o arquivo de audio", 0);
		RETURN_THROWS();
	}
	
	// seta o callback para quando terminar o audio
	ma_sound_set_end_callback(&intern->sound, onfinish_callback, intern);

	// remove o looping ao finalizar
    ma_sound_set_looping(&intern->sound, false);

	// seta como inicializado
	intern->sound_initialized = true;
}

ZEND_BEGIN_ARG_INFO_EX(arginfo_miniaudio_construct, 0, 0, 1)
	ZEND_ARG_INFO(0, filepath)
ZEND_END_ARG_INFO()

/**
 * play
 */
PHP_METHOD(MiniAudio, play)
{
	// recupera o objeto
	php_miniaudio_object *intern = Z_MINIAUDIO_P(getThis());

	// se ja tiver tocando, avisa
	if(intern->is_playing) {
		RETURN_FALSE;
	}

	// inicializa o audio / toca-le o pau marcos
	if(ma_sound_start(&intern->sound) != MA_SUCCESS) {
		zend_throw_exception(zend_ce_exception, "Erro ao iniciar a reproducao do som", 0);
		RETURN_THROWS();
	}

	intern->is_playing = true;
	RETURN_TRUE;
}

ZEND_BEGIN_ARG_INFO_EX(arginfo_miniaudio_play, 0, 0, 0)
ZEND_END_ARG_INFO()

/**
 * para a reprodução
 */
PHP_METHOD(MiniAudio, stop) {
	// recupera o objeto
	php_miniaudio_object *intern = Z_MINIAUDIO_P(getThis());

	// se nao tiver tocando, nao faz nada
	if(!intern->is_playing) {
		RETURN_FALSE;
	}

	// para o som e seta como parado
	ma_sound_stop(&intern->sound);
	intern->is_playing = false;

	// reinicia o som para o começo. diferente do miniaudio, é bem mais conveniente usar pause para iniciar de onde parou
	ma_sound_seek_to_pcm_frame(&intern->sound, 0);

	RETURN_TRUE;
}

ZEND_BEGIN_ARG_INFO_EX(arginfo_miniaudio_stop, 0, 0, 0)
ZEND_END_ARG_INFO()


/**
 * pausa a reprodução
 */
PHP_METHOD(MiniAudio, pause) {
	// recupera o objeto
	php_miniaudio_object *intern = Z_MINIAUDIO_P(getThis());

	// se nao tiver tocando, nao faz nada
	if(!intern->is_playing) {
		// inicializa o audio
		if(ma_sound_start(&intern->sound) != MA_SUCCESS) {
			zend_throw_exception(zend_ce_exception, "Erro ao iniciar a reproducao do som", 0);
			RETURN_THROWS();
		}

		intern->is_playing = true;
		RETURN_TRUE;
	}
	else {

		// para o som e seta como parado
		ma_sound_stop(&intern->sound);

		intern->is_playing = false;
		RETURN_TRUE;

	}
}

ZEND_BEGIN_ARG_INFO_EX(arginfo_miniaudio_pause, 0, 0, 0)
ZEND_END_ARG_INFO()

/**
 * metodo para verificar se está tocando
 */
PHP_METHOD(MiniAudio, isPlaying) {

	// recuperao objeto
	php_miniaudio_object *intern = Z_MINIAUDIO_P(getThis());

	
	if(intern->is_playing) {
		RETURN_TRUE;
	}
	else {
		RETURN_FALSE;
	}
}

ZEND_BEGIN_ARG_INFO_EX(arginfo_miniaudio_isplaying, 0, 0, 0)
ZEND_END_ARG_INFO()

/**
 * registra os metodos
 */
static const zend_function_entry miniaudio_methods[] = {
	PHP_ME(MiniAudio, __construct, arginfo_miniaudio_construct, ZEND_ACC_PUBLIC | ZEND_ACC_CTOR)
	PHP_ME(MiniAudio, play, arginfo_miniaudio_play, ZEND_ACC_PUBLIC)
	PHP_ME(MiniAudio, stop, arginfo_miniaudio_stop, ZEND_ACC_PUBLIC)
	PHP_ME(MiniAudio, pause, arginfo_miniaudio_pause, ZEND_ACC_PUBLIC)
	PHP_ME(MiniAudio, isPlaying, arginfo_miniaudio_isplaying, ZEND_ACC_PUBLIC)
	PHP_FE_END
};

/**
 * ao iniciar a extensão
 */
PHP_MINIT_FUNCTION(miniaudio)
{
	zend_class_entry ce;
	INIT_CLASS_ENTRY(ce, "MiniAudio", miniaudio_methods);
	miniaudio_ce = zend_register_internal_class(&ce);

	// cria o objeto global
	miniaudio_ce->create_object = miniaudio_object_create;
	memcpy(&miniaudio_object_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
	miniaudio_object_handlers.offset = XtOffsetOf(php_miniaudio_object, std);
	miniaudio_object_handlers.free_obj = miniaudio_object_free;

	// inicializa o miniaudio
	if (ma_engine_init(NULL, &global_audio_engine) == MA_SUCCESS) {
		global_engine_initialized = true;
	}

	return SUCCESS;
}

/**
 * ao finalizar a extensão
 */
PHP_MSHUTDOWN_FUNCTION(miniaudio)
{
	if (global_engine_initialized) {
		ma_engine_uninit(&global_audio_engine);
		global_engine_initialized = false;
	}
	return SUCCESS;
}

/**
 * junta tudo
 */
zend_module_entry miniaudio_module_entry = {
	STANDARD_MODULE_HEADER,
	PHP_MINIAUDIO_EXTNAME,
	NULL,
	PHP_MINIT(miniaudio),
	PHP_MSHUTDOWN(miniaudio),
	NULL,
	NULL,
	NULL,
	PHP_MINIAUDIO_VERSION,
	STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_MINIAUDIO
ZEND_GET_MODULE(miniaudio)
#endif