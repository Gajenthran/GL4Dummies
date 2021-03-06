#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <GL4D/gl4df.h>
#include <GL4D/gl4du.h>
#include <GL4D/gl4duw_SDL2.h>
#include "mobile.h"

static void init(void);
static void draw(void);
static void quit(void);
/*!\brief dimensions de la fenêtre */
static int _windowWidth = 800, _windowHeight = 600;
/*!\brief identifiant du programme de coloriage GLSL */
static GLuint _shPID = 0;
/*!\brief identifiant du programme shadow map GLSL */
static GLuint _smPID = 0;
/*!\brief quelques objets géométriques */
static GLuint _sphere = 0, _quad = 0;
/*!\brief scale du plan */
static GLfloat _plan_s = 10.0f;
/*!\brief Framebuffer Object */
static GLuint _fbo = 0;
/*!\brief Texture recevant la couleur */
static GLuint _colorTex = 0;
/*!\brief Texture recevant la profondeur */
static GLuint _depthTex = 0;
/*!\brief Texture recevant les identifiants d'objets */
static GLuint _idTex = 0;
/*!\brief Texture recevant la shadow map */
static GLuint _smTex = 0;
/*!\brief position de la lumière, relative aux objets */
static GLfloat _lumpos[] = { -9, 9, 0, 1 };
#define SHADOW_MAP_SIDE 512

/*!\brief La fonction principale créé la fenêtre d'affichage,
 * initialise GL et les données, affecte les fonctions d'événements et
 * lance la boucle principale d'affichage.*/
int main(int argc, char ** argv) {
  if(!gl4duwCreateWindow(argc, argv, "GL4D - Picking", 0, 0, _windowWidth, _windowHeight, GL4DW_SHOWN))
    return 1;
  init();
  atexit(quit);
  gl4duwIdleFunc(mobileMove);
  gl4duwDisplayFunc(draw);
  gl4duwMainLoop();
  return 0;
}
/*!\brief initialise les paramètres OpenGL */
static void init(void) {
  glEnable(GL_DEPTH_TEST);
  _shPID  = gl4duCreateProgram("<vs>shaders/basic.vs", "<fs>shaders/basic.fs", NULL);
  _smPID  = gl4duCreateProgram("<vs>shaders/shadowMap.vs", "<fs>shaders/shadowMap.fs", NULL);
  gl4duGenMatrix(GL_FLOAT, "modelMatrix");
  gl4duGenMatrix(GL_FLOAT, "lightViewMatrix");
  gl4duGenMatrix(GL_FLOAT, "lightProjectionMatrix");
  gl4duGenMatrix(GL_FLOAT, "cameraViewMatrix");
  gl4duGenMatrix(GL_FLOAT, "cameraProjectionMatrix");
  gl4duGenMatrix(GL_FLOAT, "cameraPVMMatrix");

  glViewport(0, 0, _windowWidth, _windowHeight);
  gl4duBindMatrix("lightProjectionMatrix");
  gl4duLoadIdentityf();
  gl4duFrustumf(-1, 1, -1, 1, 1.5, 50.0);
  gl4duBindMatrix("cameraProjectionMatrix");
  gl4duLoadIdentityf();
  gl4duFrustumf(-0.5, 0.5, -0.5 * _windowHeight / _windowWidth, 0.5 * _windowHeight / _windowWidth, 1.0, 50.0);
  gl4duBindMatrix("modelMatrix");

  _sphere = gl4dgGenSpheref(30, 30);
  _quad = gl4dgGenQuadf();
  mobileInit(1, _plan_s, _plan_s);

  /* Création et paramétrage de la Texture de shadow map */
  glGenTextures(1, &_smTex);
  glBindTexture(GL_TEXTURE_2D, _smTex);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_MAP_SIDE, SHADOW_MAP_SIDE, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

  /* Création et paramétrage de la Texture recevant la couleur */
  glGenTextures(1, &_colorTex);
  glBindTexture(GL_TEXTURE_2D, _colorTex);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _windowWidth, _windowHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

  /* Création et paramétrage de la Texture recevant la profondeur */
  glGenTextures(1, &_depthTex);
  glBindTexture(GL_TEXTURE_2D, _depthTex);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, _windowWidth, _windowHeight, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, NULL);

  /* Création et paramétrage de la Texture recevant les identifiants d'objets */
  glGenTextures(1, &_idTex);
  glBindTexture(GL_TEXTURE_2D, _idTex);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, _windowWidth, _windowHeight, 0, GL_RED, GL_UNSIGNED_INT, NULL);

  /* Création du Framebuffer Object */
  glGenFramebuffers(1, &_fbo);
}

/*!\brief la scène est soit dessinée du point de vu de la lumière (sm
 *  = GL_TRUE donc shadow map) soit dessinée du point de vue de la
 *  caméra */
static inline void scene(GLboolean sm) {
  glEnable(GL_CULL_FACE);
  int time = SDL_GetTicks();
  if(sm) {
    glCullFace(GL_FRONT);
    glUseProgram(_smPID);
    gl4duBindMatrix("lightViewMatrix");
    gl4duLoadIdentityf();
    gl4duLookAtf(_lumpos[0], _lumpos[1], _lumpos[2], 0, 2, 0, 0, 1, 0);
    gl4duBindMatrix("modelMatrix");
    gl4duLoadIdentityf();
  } else {
    GLfloat white[] = {255, 255, 255, 0}, lp[4], *mat;
    glCullFace(GL_BACK);
    glUseProgram(_shPID);
    glEnable(GL_TEXTURE_2D);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _smTex);
    glUniform1i(glGetUniformLocation(_shPID, "smTex"), 0);
    gl4duBindMatrix("cameraViewMatrix");
    gl4duLoadIdentityf();
    gl4duLookAtf(0, 4, 22, 0, 2, 0, 0, 1, 0);
    /* lumière positionnelle */
    mat = gl4duGetMatrixData();
    MMAT4XVEC4(lp, mat, _lumpos);
    MVEC4WEIGHT(lp);
    glUniform4fv(glGetUniformLocation(_shPID, "lumpos"), 1, lp);
    gl4duBindMatrix("modelMatrix");
    gl4duLoadIdentityf();
    gl4duPushMatrix(); {
      gl4duTranslatef(_lumpos[0], _lumpos[1], _lumpos[2]);
      gl4duSendMatrices();
    } gl4duPopMatrix();
    glUniform1i(glGetUniformLocation(_shPID, "id"), 2);
    // gl4dgDraw(_sphere);
    glUniform4fv(glGetUniformLocation(_shPID, "couleur"), 1, white);
    glUniform1i(glGetUniformLocation(_shPID, "id"), 1);
    glUniform1i(glGetUniformLocation(_shPID, "time"), time);
  }
  gl4duPushMatrix(); {
    gl4duRotatef(-90, 1, 0, 0);
    gl4duScalef(_plan_s, _plan_s, _plan_s);
    gl4duSendMatrices();
  } gl4duPopMatrix();
  gl4dgDraw(_quad);
  gl4duSendMatrices();
  mobileDraw(_sphere);
}

/*!\brief dessine dans le contexte OpenGL actif. */
static void draw(void) {
  GLenum renderings[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
  glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
  /* désactiver le rendu de couleur et ne laisser que le depth, dans _smTex */
  glDrawBuffer(GL_NONE);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, _smTex, 0);
  /* viewport de la shadow map et dessin de la scène du point de vue de la lumière */
  glViewport(0, 0, SHADOW_MAP_SIDE, SHADOW_MAP_SIDE);
  glClear(GL_DEPTH_BUFFER_BIT);
  scene(GL_TRUE);
  /* paramétrer le fbo pour 2 rendus couleurs + un (autre) depth */
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _colorTex, 0);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, _idTex, 0);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, _depthTex, 0);
  glViewport(0, 0, _windowWidth, _windowHeight);
  /* un seul rendu GL_COLOR_ATTACHMENT1 + effacement 0 */
  glDrawBuffers(1, &renderings[1]);
  glClearColor(0, 0, 0, 0);
  glClear(GL_COLOR_BUFFER_BIT);
  /* un seul rendu GL_COLOR_ATTACHMENT0 + effacement couleur et depth */
  glDrawBuffers(1, renderings);
  glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  /* deux rendus GL_COLOR_ATTACHMENT0 et GL_COLOR_ATTACHMENT1 */
  glDrawBuffers(2, renderings);

  scene(GL_FALSE);

  /* copie du fbo à l'écran */
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
  glBlitFramebuffer(0, 0, _windowWidth, _windowHeight, 0, 0, _windowWidth, _windowHeight, GL_COLOR_BUFFER_BIT, GL_LINEAR);
  glBlitFramebuffer(0, 0, _windowWidth, _windowHeight, 0, 0, _windowWidth, _windowHeight, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
}
/*!\brief appelée au moment de sortir du programme (atexit), libère les éléments utilisés */
static void quit(void) {
  if(_fbo) {
    glDeleteTextures(1, &_colorTex);
    glDeleteTextures(1, &_depthTex);
    glDeleteTextures(1, &_idTex);
    glDeleteTextures(1, &_smTex);
    glDeleteFramebuffers(1, &_fbo);
    _fbo = 0;
  }
  gl4duClean(GL4DU_ALL);
}
