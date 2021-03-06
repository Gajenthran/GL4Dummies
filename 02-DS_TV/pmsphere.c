#include <assert.h>
#include <GL4D/gl4du.h>
#include <GL4D/gl4duw_SDL2.h>
#include <SDL_image.h>
#include <SDL_mixer.h>
#include <GL4D/gl4dh.h>
#include "audioHelper.h"

static void  init(int w, int h);
static void  draw(void);
static void  quit(void);

/* !\brief écran de la démo */
static GLuint _screen = 0;
/* !\brief dimensions de la démo */
static int _w, _h;
/*!\brief identifiant de la texture */
static GLuint _tId = 0;
/*!\brief identifiant du programme GLSL */
static GLuint _pId = 0;
/*!\brief identifiant de la sphère de GL4Dummies */
static GLuint _sphere = 0;
/*!\brief dimension de la sphère "pacman" */
static GLfloat _sphereSize = 0.10;
/*!\brief coordonnées de la sphère "pacman" */
static GLfloat _spherePos[3] = {0.0, 1.0, -3.0};
/*!\brief nombre de sphères "étoiles" */
static int _nbStars = 200;
/*!\brief coordonnées des sphères "étoiles" */
static GLfloat _starsPos[200] = {0.0};
/*!\brief longitudes et latitudes de la sphère */
static GLuint _longitudes = 200, _latitudes = 200;
/*!\brief coordonnées de la lumière */
static GLfloat _lumPos0[4] = {-15.1, 20.0, 20.7, 1.0};
/*!\brief fichier contenant la texture */
static const char * _texture_filename = "images/pacman.png";
/*!\brief nom de la texture */
static const char * _sampler_name = "pacman";
/*!\brief précision concernant la représentation de la texture */
static GLfloat _pixelPrec = 1.0;
/* !\brief basses de la démo */
static int _basses = 0;
/*!\brief mode _pixel pour la pixellisation de la texture */
static int _pixel = 0;
/*!\brief mode _swirl pour le mélange des éléments d'une texture */
static int _swirl = 0;
/*!\brief état de la démo */
static int _state = 0;

/*!\brief initialise les paramètres OpenGL et les données */
static void init(int w, int h) {
  _w = w; _h = h;
  int i;

  /* initialise les images */
  if(!_tId) {
    glGenTextures(1, &_tId);
    SDL_Surface * t;
    glBindTexture(GL_TEXTURE_2D, _tId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    if( (t = IMG_Load(_texture_filename)) != NULL ) {
      int mode = t->format->BytesPerPixel == 4 ? GL_RGBA : GL_RGB;    
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, t->w, t->h, 0, mode, GL_UNSIGNED_BYTE, t->pixels);
      SDL_FreeSurface(t);
    } else {
      fprintf(stderr, "can't open file %s : %s\n", _texture_filename, SDL_GetError());
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    }
  }

  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  _pId  = gl4duCreateProgram("<vs>shaders/pmsphere.vs", "<fs>shaders/pmsphere.fs", NULL);
  gl4duGenMatrix(GL_FLOAT, "modelViewMatrix");
  gl4duGenMatrix(GL_FLOAT, "projectionMatrix");
  _sphere = gl4dgGenSpheref(_longitudes, _latitudes);

  /* initialise les coordonnées des sphères "étoiles" */
  for(i = 0; i < _nbStars; i++)
    _starsPos[i] = gl4dmURand() * 3.0 - 1.5;
}

/*!\brief dessine dans le contexte OpenGL actif. */
static void draw(void) {
  static int prev_basses = 0.0;
  static GLfloat a0 = 0.0;
  static Uint32 t0 = 0;
  int i;
  GLint vp[4];
  GLfloat dt = 0.0, steps[2] = {1.0f / _w, 1.0f / _h};
  GLfloat lumPos[4], *mat;
  Uint32 t = SDL_GetTicks();
  dt = (t - t0) / 1000.0;
  t0 = t;

  GLboolean gdt = glIsEnabled(GL_DEPTH_TEST);
  glGetIntegerv(GL_VIEWPORT, vp);
  gl4duBindMatrix("projectionMatrix");
  gl4duLoadIdentityf();
  gl4duFrustumf(-0.5, 0.5, -0.5 * vp[3] / vp[2], 0.5 * vp[3] / vp[2], 1.0, 1000.0);
  gl4duBindMatrix("modelViewMatrix");
  
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glDisable(GL_CULL_FACE);
  glEnable(GL_DEPTH_TEST);
  glDisable(GL_BLEND);
  gl4duBindMatrix("modelViewMatrix");
  gl4duLoadIdentityf();
  mat = gl4duGetMatrixData();
  MMAT4XVEC4(lumPos, mat, _lumPos0);

  /* dessine la sphère "pacman" */
  glUseProgram(_pId);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, _tId);
  gl4duPushMatrix(); {
    gl4duTranslatef(_spherePos[0], _spherePos[1], _spherePos[2]);
    gl4duRotatef(a0, 0, 1, 0);
    gl4duScalef(_sphereSize, _sphereSize, _sphereSize);
    glUniform1i(glGetUniformLocation(_pId, _sampler_name), 0);
    glUniform1i(glGetUniformLocation(_pId, "id"), 1);
    glUniform1i(glGetUniformLocation(_pId, "basses"), _basses);
    glUniform1i(glGetUniformLocation(_pId, "swirl"), _swirl);
    glUniform1i(glGetUniformLocation(_pId, "pixel"), _pixel);
    glUniform1i(glGetUniformLocation(_pId, "state"), _state);
    glUniform1f(glGetUniformLocation(_pId, "pixelPrec"), _pixelPrec);
    glUniform1i(glGetUniformLocation(_pId, "time"), t);
    glUniform2fv(glGetUniformLocation(_pId, "steps"), 1, steps);
    glUniform4fv(glGetUniformLocation(_pId, "lumPos"), 1, lumPos);
    gl4duSendMatrices();
  } gl4duPopMatrix();
  gl4dgDraw(_sphere);
  
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, 0);

  /* dessine les sphères "étoiles" */
  for(i = 0; i < _nbStars/2; i++) {
    gl4duPushMatrix(); {
      gl4duTranslatef(_starsPos[i], _starsPos[i + 1], -3.0);
      gl4duScalef(_basses * 0.001, _basses * 0.001, _basses * 0.001);
      glUniform1i(glGetUniformLocation(_pId, "id"), 2);
      gl4duSendMatrices();
    } gl4duPopMatrix();
    if(_state >= 4 || _state < 0) gl4dgDraw(_sphere);
  } 

  a0 += 1000.0 * dt / 24.0;
  if(prev_basses == 5 && _basses == 6 && _state < 3) _state++;
  if(_spherePos[1] > 0) {  _swirl = 1; _spherePos[1] -= 0.0065; }
  if(_state >= 3 && _sphereSize < 0.50) { _sphereSize += 0.1; }
  if(_basses >= 13) _state++;
  if(_state == 3) { _swirl = 0; _pixel = 1; a0 = 270.0; _pixelPrec += 0.5; }
  if(_state >= 4) { _pixel = 0; }
  if(_state >= 14 && _basses >= prev_basses) { _state = -99; }
  prev_basses = _basses;

  if(!gdt)
    glDisable(GL_DEPTH_TEST);
}

/* !\brief libère les éléments OpenGL utilisés */
static void quit(void) {
  if(_tId) {
    glDeleteTextures(1, &_tId);
    _tId = 0;
  }

  if(_screen) {
    gl4dpSetScreen(_screen);
    gl4dpDeleteScreen();
    _screen = 0;
  }
}

void pmsphere(int state) {
  Sint16 * s;
  switch(state) {
  case GL4DH_INIT:
    _screen = gl4dpInitScreen();
    init(gl4dpGetWidth(), gl4dpGetHeight());
    return;
  case GL4DH_FREE:
    quit();
    return;
  case GL4DH_UPDATE_WITH_AUDIO:
    s = (Sint16 *)ahGetAudioStream();
    _basses = ahGetAudioStreamFreq();
    return;
  default:
    draw();
    return;
  }
}
