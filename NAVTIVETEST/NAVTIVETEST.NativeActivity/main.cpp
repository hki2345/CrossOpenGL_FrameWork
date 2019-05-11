/*
* Copyright (C) 2010 The Android Open Source Project
*
* Apache ���̼��� 2.0 ����(���� "���̼���")�� ���� ���̼����� �ο��˴ϴ�.
* ���̼����� �ؼ����� ������ �� ������ ����� �� �����ϴ�.
* ���̼����� �纻��
*
*      http://www.apache.org/licenses/LICENSE-2.0���� ���� �� �ֽ��ϴ�.
*
* ���� ������ ������ ���� �ʿ��ϰų� �������� �������� �ʴ� �̻�
* ���̼����� ���� �����Ǵ� ����Ʈ����� "�ִ� �״��",
* ����� �Ǵ� �������̵� ��� ������ �����̳� ���� ���� �����˴ϴ�.
* ���̼����� ���� Ư�� ����� ���� �� ���ѿ� ���� ������
* ���̼����� �����ϼ���.
*
*/

#include <malloc.h>

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "AndroidProject1.NativeActivity", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "AndroidProject1.NativeActivity", __VA_ARGS__))

/**
* ����� ���� �������Դϴ�.
*/
struct saved_state {
	float angle;
	int32_t x;
	int32_t y;
};

/**
* �ۿ� ���� ���� �����Դϴ�.
*/
struct engine {
	struct android_app* app;

	ASensorManager* sensorManager;
	const ASensor* accelerometerSensor;
	ASensorEventQueue* sensorEventQueue;

	int animating;
	EGLDisplay display;
	EGLSurface surface;
	EGLContext context;
	int32_t width;
	int32_t height;
	struct saved_state state;
};

/**
* ���� ���÷��̿� ���� EGL ���ؽ�Ʈ�� �ʱ�ȭ�մϴ�.
*/
static int engine_init_display(struct engine* engine) {
	// OpenGL ES �� EGL �ʱ�ȭ

	/*
	* ���⿡�� ���ϴ� ������ Ư���� �����մϴ�.
	* �Ʒ����� ȭ�� â�� ȣȯ�Ǵ� 
	* ���� ���� ��Ҵ� �ּ� 8��Ʈ�� ���Ե� EGLConfig�� �����߽��ϴ�.
	*/
	const EGLint attribs[] = {
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_BLUE_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_RED_SIZE, 8,
		EGL_NONE
	};
	EGLint w, h, format;
	EGLint numConfigs;
	EGLConfig config;
	EGLSurface surface;
	EGLContext context;

	EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

	eglInitialize(display, 0, 0);

	/* ���⿡�� ���ø����̼��� ���ϴ� ������ �����մϴ�. �� ���ÿ���
	* ���ؿ� ��ġ�ϴ� ù ��° EGLConfig�� �����ϴ�
	* ���� ����ȭ�� ���� ������ ���ԵǾ� �ֽ��ϴ�. */
	eglChooseConfig(display, attribs, &config, 1, &numConfigs);

	/* EGL_NATIVE_VISUAL_ID�� EGLConfig�� Ư������
	* ANativeWindow_setBuffersGeometry()���� ���εǵ��� �����մϴ�.
	* EGLConfig�� �����ϸ� EGL_NATIVE_VISUAL_ID�� ����Ͽ� 
	* ANativeWindow ���۰� ��ġ�ϵ��� �����ϰ� �ٽ� ������ �� �ֽ��ϴ�. */
	eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format);

	ANativeWindow_setBuffersGeometry(engine->app->window, 0, 0, format);

	surface = eglCreateWindowSurface(display, config, engine->app->window, NULL);
	context = eglCreateContext(display, config, NULL, NULL);

	if (eglMakeCurrent(display, surface, surface, context) == EGL_FALSE) {
		LOGW("Unable to eglMakeCurrent");
		return -1;
	}

	eglQuerySurface(display, surface, EGL_WIDTH, &w);
	eglQuerySurface(display, surface, EGL_HEIGHT, &h);

	engine->display = display;
	engine->context = context;
	engine->surface = surface;
	engine->width = w;
	engine->height = h;
	engine->state.angle = 0;

	// GL ���¸� �ʱ�ȭ�մϴ�.
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
	glEnable(GL_CULL_FACE);
	glShadeModel(GL_SMOOTH);
	glDisable(GL_DEPTH_TEST);

	return 0;
}

/**
* ���÷����� ���� �������Դϴ�.
*/
static void engine_draw_frame(struct engine* engine) {
	if (engine->display == NULL) {
		// ���÷��̰� �����ϴ�.
		return;
	}

	// ȭ���� �������� ä��ϴ�.
	glClearColor(((float)engine->state.x) / engine->width, engine->state.angle,
		((float)engine->state.y) / engine->height, 1);
	glClear(GL_COLOR_BUFFER_BIT);

	eglSwapBuffers(engine->display, engine->surface);
}

/**
* ���� ���÷��̿� ����� EGL ���ؽ�Ʈ�� �����մϴ�.
*/
static void engine_term_display(struct engine* engine) {
	if (engine->display != EGL_NO_DISPLAY) {
		eglMakeCurrent(engine->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
		if (engine->context != EGL_NO_CONTEXT) {
			eglDestroyContext(engine->display, engine->context);
		}
		if (engine->surface != EGL_NO_SURFACE) {
			eglDestroySurface(engine->display, engine->surface);
		}
		eglTerminate(engine->display);
	}
	engine->animating = 0;
	engine->display = EGL_NO_DISPLAY;
	engine->context = EGL_NO_CONTEXT;
	engine->surface = EGL_NO_SURFACE;
}

/**
* ���� �Է� �̺�Ʈ�� ó���մϴ�.
*/
static int32_t engine_handle_input(struct android_app* app, AInputEvent* event) {
	struct engine* engine = (struct engine*)app->userData;
	if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
		engine->state.x = AMotionEvent_getX(event, 0);
		engine->state.y = AMotionEvent_getY(event, 0);
		return 1;
	}
	return 0;
}

/**
* ���� �� ����� ó���մϴ�.
*/
static void engine_handle_cmd(struct android_app* app, int32_t cmd) {
	struct engine* engine = (struct engine*)app->userData;
	switch (cmd) {
	case APP_CMD_SAVE_STATE:
		// �ý��ۿ��� ���� ���¸� �����ϵ��� ��û�߽��ϴ�. �����ϼ���.
		engine->app->savedState = malloc(sizeof(struct saved_state));
		*((struct saved_state*)engine->app->savedState) = engine->state;
		engine->app->savedStateSize = sizeof(struct saved_state);
		break;
	case APP_CMD_INIT_WINDOW:
		// â�� ǥ�õǾ� �غ� ���ƽ��ϴ�.
		if (engine->app->window != NULL) {
			engine_init_display(engine);
			engine_draw_frame(engine);
		}
		break;
	case APP_CMD_TERM_WINDOW:
		// â�� ����ų� �ݾ� �����մϴ�.
		engine_term_display(engine);
		break;
	case APP_CMD_GAINED_FOCUS:
		// �ۿ� ��Ŀ���� ������ ���ӵ��� ����͸��� �����մϴ�.
		if (engine->accelerometerSensor != NULL) {
			ASensorEventQueue_enableSensor(engine->sensorEventQueue,
				engine->accelerometerSensor);
			// �ʴ� 60���� �̺�Ʈ�� ������ �� �ֽ��ϴ�.
			ASensorEventQueue_setEventRate(engine->sensorEventQueue,
				engine->accelerometerSensor, (1000L / 60) * 1000);
		}
		break;
	case APP_CMD_LOST_FOCUS:
		// �ۿ��� ��Ŀ���� ������� ���ӵ��� ����͸��� �����˴ϴ�.
		// ������� �ʴ� ���� ���͸��� �����ϱ� ���� ��ġ�Դϴ�.
		if (engine->accelerometerSensor != NULL) {
			ASensorEventQueue_disableSensor(engine->sensorEventQueue,
				engine->accelerometerSensor);
		}
		// �ִϸ��̼ǵ� �����˴ϴ�.
		engine->animating = 0;
		engine_draw_frame(engine);
		break;
	}
}

/**
* android_native_app_glue�� ����ϴ� ����Ƽ�� ���ø����̼���
* �� �������Դϴ�. ��ü �����忡�� ����Ǹ�, �Է� �̺�Ʈ��
* �ް� �ٸ� �۾��� �����ϴ� ��ü �̺�Ʈ ������ �����մϴ�.
*/
void android_main(struct android_app* state) {
	struct engine engine;

	memset(&engine, 0, sizeof(engine));
	state->userData = &engine;
	state->onAppCmd = engine_handle_cmd;
	state->onInputEvent = engine_handle_input;
	engine.app = state;

	// ���ӵ��� ����͸��� �غ��մϴ�.
	engine.sensorManager = ASensorManager_getInstance();
	engine.accelerometerSensor = ASensorManager_getDefaultSensor(engine.sensorManager,
		ASENSOR_TYPE_ACCELEROMETER);
	engine.sensorEventQueue = ASensorManager_createEventQueue(engine.sensorManager,
		state->looper, LOOPER_ID_USER, NULL, NULL);

	if (state->savedState != NULL) {
		// ������ ����� ���·� ���۵Ǹ�, �� �������� �����˴ϴ�.
		engine.state = *(struct saved_state*)state->savedState;
	}

	engine.animating = 1;

	//������ �۾��� ����ϸ鼭 ������ �����մϴ�.

	while (1) {
		// ���� ���� ��� �̺�Ʈ�� �н��ϴ�.
		int ident;
		int events;
		struct android_poll_source* source;

		// �ִϸ��̼��� �������� ������ �̺�Ʈ ��⸦ ���������� �����մϴ�.
		// �ִϸ��̼��� �����ϸ� ��� �̺�Ʈ�� ���� ������ ������ ������ ����
		// ����ؼ� �ִϸ��̼��� ���� �������� �׸��ϴ�.
		while ((ident = ALooper_pollAll(engine.animating ? 0 : -1, NULL, &events,
			(void**)&source)) >= 0) {

			// �� �̺�Ʈ�� ó���մϴ�.
			if (source != NULL) {
				source->process(state, source);
			}

			// ������ �����Ͱ� ������ �ٷ� ó���˴ϴ�.
			if (ident == LOOPER_ID_USER) {
				if (engine.accelerometerSensor != NULL) {
					ASensorEvent event;
					while (ASensorEventQueue_getEvents(engine.sensorEventQueue,
						&event, 1) > 0) {
						LOGI("accelerometer: x=%f y=%f z=%f",
							event.acceleration.x, event.acceleration.y,
							event.acceleration.z);
					}
				}
			}

			// ���� ������ Ȯ���մϴ�.
			if (state->destroyRequested != 0) {
				engine_term_display(&engine);
				return;
			}
		}

		if (engine.animating) {
			// �̺�Ʈ�� ������ �� ���� �ִϸ��̼� �������� �׸��ϴ�.
			engine.state.angle += .01f;
			if (engine.state.angle > 1) {
				engine.state.angle = 0;
			}

			// �׸���� ȭ�� ������Ʈ �ӵ��� ������ �����Ƿ�
			// ���⿡���� Ÿ�̹��� ����� �ʿ䰡 �����ϴ�.
			engine_draw_frame(&engine);
		}
	}
}
