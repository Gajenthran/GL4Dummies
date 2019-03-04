/*!\file window.c
 *
 * \brief Walk in a labyrinth with floor marking and compass.
 *
 * \author Farès BELHADJ, amsi@ai.univ-paris8.fr
 * \date March 05 2018
 */
#include <GL4D/gl4dg.h>
#include <math.h>
#include <GL4D/gl4dp.h>
#include <GL4D/gl4duw_SDL2.h>
#include <GL4D/gl4du.h>
#include <SDL_image.h>

static void quit(void);
static void initGL(void);
static void initData(void);
static void resize(int w, int h);
static void idle(void);
static void keydown(int keycode);
static void keyup(int keycode);
static void pmotion(int x, int y);
static int  collision(void);
static void spawnItem(void);
static void draw(void);

/* from makeLabyrinth.c */
extern unsigned int * labyrinth(int w, int h);

/*!\brief opened window width and height */
static int _wW = 800, _wH = 600;
/*!\brief mouse position (modified by pmotion function) */
static int _xm = 400, _ym = 300;
/*!\brief labyrinth to generate */
static GLuint * _labyrinth = NULL;
/*!\brief labyrinth side */
static GLuint _lab_side = 15;
/*!\brief Quad geometry Id  */
static GLuint _plane = 0;
/*!\brief Cube geometry Id  */
static GLuint _cube = 0;
/*!\brief GLSL program Id */
static GLuint _pId = 0;
/*!\brief plane texture Id */
static GLuint _planeTexId = 0;
/*!\brief wall texture Id */
static GLuint _wallTexId = 0;
/*!\brief compass texture Id */
static GLuint _compassTexId = 0;
/*!\brief item texture Id */
static GLuint _itemTexId = 0;
/*!\brief plane scale factor */
static GLfloat _planeScale = 100.0f;
/*!\brief boolean to toggle anisotropic filtering */
static GLboolean _anisotropic = GL_FALSE;
/*!\brief boolean to toggle mipmapping */
static GLboolean _mipmap = GL_FALSE;
/*!\brief item filename to load */
static const char * _item_filename = "img/box.png";
static const char * _wall_filename = "img/wall.png";

/*!\brief enum that index keyboard mapping for direction commands */
enum kyes_t {
  KLEFT = 0,
  KRIGHT,
  KUP,
  KDOWN
};

enum typecell {
  ROAD = 0,
  WALL = -1,
  ITEM = 1
};

/*!\brief virtual keyboard for direction commands */
static GLuint _keys[] = {0, 0, 0, 0};

typedef struct cam_t cam_t;
/*!\brief a data structure for storing camera position and
 * orientation */
struct cam_t {
  GLfloat x, z;
  GLfloat theta;
  GLfloat bbox;
};

typedef struct item_t item_t;
struct item_t {
  GLfloat x, z, w, h;
};

/*!\brief the used camera */
static cam_t _cam = {0, 0, 0, 1.0};
static item_t _item = {0, 0};
static int _collision_dir[] = {0, 0, 0, 0};

/*!\brief creates the window, initializes OpenGL parameters,
 * initializes data and maps callback functions */
int main(int argc, char ** argv) {
  if(!gl4duwCreateWindow(argc, argv, "GL4Dummies", 10, 10,
                         _wW, _wH, GL4DW_RESIZABLE | GL4DW_SHOWN))
    return 1;
  initGL();
  initData();
  atexit(quit);
  gl4duwResizeFunc(resize);
  gl4duwKeyUpFunc(keyup);
  gl4duwKeyDownFunc(keydown);
  gl4duwPassiveMotionFunc(pmotion);
  gl4duwDisplayFunc(draw);
  gl4duwIdleFunc(idle);
  gl4duwMainLoop();
  return 0;
}

/*!\brief initializes OpenGL parameters :
 *
 * the clear color, enables face culling, enables blending and sets
 * its blending function, enables the depth test and 2D textures,
 * creates the program shader, the model-view matrix and the
 * projection matrix and finally calls resize that sets the projection
 * matrix and the viewport.
 */
static void initGL(void) {
  glClearColor(0.0f, 0.4f, 0.9f, 0.0f);
  glEnable(GL_CULL_FACE);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_TEXTURE_2D);
  _pId  = gl4duCreateProgram("<vs>shaders/basic.vs", "<fs>shaders/basic.fs", NULL);
  gl4duGenMatrix(GL_FLOAT, "modelMatrix");
  gl4duGenMatrix(GL_FLOAT, "viewMatrix");
  gl4duGenMatrix(GL_FLOAT, "projectionMatrix");
  resize(_wW, _wH);
}

/*!\brief initializes data : 
 *
 * creates 3D objects (plane and sphere) and 2D textures.
 */
static void initData(void) {
  /* a red-white texture used to draw a compass */
  GLuint northsouth[] = {(255 << 24) + 255, -1};
  /* generates a quad using GL4Dummies */
   _plane = gl4dgGenQuadf();
  /* generates a cube using GL4Dummies */
  _cube = gl4dgGenCubef();

  /* creation and parametrization of the plane texture */
  glGenTextures(1, &_planeTexId);
  glBindTexture(GL_TEXTURE_2D, _planeTexId);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  _labyrinth = labyrinth(_lab_side, _lab_side);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _lab_side, _lab_side, 0, GL_RGBA, GL_UNSIGNED_BYTE, _labyrinth);

  /* creation and parametrization of the compass texture */
  glGenTextures(1, &_compassTexId);
  glBindTexture(GL_TEXTURE_2D, _compassTexId);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, northsouth);

  SDL_Surface * t;
  int mode;
  /* creation and parametrization of the wall texture */
  glGenTextures(1, &_wallTexId);
  glBindTexture(GL_TEXTURE_2D, _wallTexId);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  if( (t = IMG_Load(_wall_filename)) != NULL ) {
    mode = t->format->BytesPerPixel == 4 ? GL_RGBA : GL_RGB;
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, t->w, t->h, 0, mode, GL_UNSIGNED_BYTE, t->pixels);
    SDL_FreeSurface(t);
  } else {
    fprintf(stderr, "can't open file %s : %s\n", _wall_filename, SDL_GetError());
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  }

  glGenTextures(1, &_itemTexId);
  glBindTexture(GL_TEXTURE_2D, _itemTexId);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  if( (t = IMG_Load(_item_filename)) != NULL ) {
    mode = t->format->BytesPerPixel == 4 ? GL_RGBA : GL_RGB;
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, t->w, t->h, 0, mode, GL_UNSIGNED_BYTE, t->pixels);
    SDL_FreeSurface(t);
  } else {
    fprintf(stderr, "can't open file %s : %s\n", _item_filename, SDL_GetError());
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  }

  spawnItem();
  glBindTexture(GL_TEXTURE_2D, 0);
}

/*!\brief function called by GL4Dummies' loop at resize. Sets the
 *  projection matrix and the viewport according to the given width
 *  and height.
 * \param w window width
 * \param h window height
 */
static void resize(int w, int h) {
  _wW = w; 
  _wH = h;
  glViewport(0, 0, _wW, _wH);
  gl4duBindMatrix("projectionMatrix");
  gl4duLoadIdentityf();
  gl4duFrustumf(-0.5, 0.5, -0.5 * _wH / _wW, 0.5 * _wH / _wW, 1.0, _planeScale + 1.0);
}

/*!\brief Help to carry out your work. Tracking the position in the
 * world with the position on the map.
 */
static void updatePosition(void) {
  GLfloat xf, zf;
  static int xi = -1, zi = -1;
  /* translate to middle */
  xf = _cam.x + _planeScale;
  zf = -_cam.z + _planeScale;
  /* scale to 1.0 x 1.0 */
  xf = xf / (2.0f * _planeScale);
  zf = zf / (2.0f * _planeScale);
  /* rescale to _lab_side x _lab_side */
  xf = xf * _lab_side;
  zf = zf * _lab_side;
  /* re-set previous position to black and the new one to red */
  xi = (int)xf;
  zi = (int)zf;
  // printf("player: %f - %f\n", xf, zf);
  if(xi >= 0 && xi < _lab_side && zi >= 0 && zi < _lab_side) {
    printf("item.x : %f - xf : %f\n", _item.x, xf);
    printf("item.z : %f - zf : %f\n", _item.z, zf);
    GLfloat dx = xf - _item.x;
    GLfloat dz = zf - _item.z;
    double dist = sqrt(dx * dx + dz * dz);
    printf("dist: %f\n\n", dist);
    if(_labyrinth[zi * _lab_side + xi] == 2 && dist < 1.0) {
      _labyrinth[zi * _lab_side + xi] = 0;
      glBindTexture(GL_TEXTURE_2D, _planeTexId);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _lab_side, _lab_side, 0, GL_RGBA, GL_UNSIGNED_BYTE, _labyrinth);
      spawnItem();  
    }
  }
}

static void spawnItem(void) {
  GLfloat x, z;
  do {
    x = gl4dmURand() * _lab_side;
    z = gl4dmURand() * _lab_side;
  } while(_labyrinth[(int)z * _lab_side + (int)x] == -1);
  _item.x = x; 
  _item.z = z;
  _item.w = _item.h = 1.0;
  _labyrinth[(int)z * _lab_side + (int)x] = 2;
}

static int collision(void) {
  GLfloat xf, zf;
  int xi = -1, zi = -1, i;
  int col = 0;

  GLfloat dir[4][2] = {
    { -cos(_cam.theta), sin(_cam.theta)},
    {  cos(_cam.theta), sin(_cam.theta)},
    { -sin(_cam.theta), cos(_cam.theta)},
    {  sin(_cam.theta), -cos(_cam.theta)}
  };

  for(i = 0; i < 4; i++) {
    xf = _cam.x + _planeScale + dir[i][0] * _cam.bbox; 
    zf = -_cam.z + _planeScale + dir[i][1] * _cam.bbox/4;
    /* scale to 1.0 x 1.0 */
    xf = xf / (2.0f * _planeScale);
    zf = zf / (2.0f * _planeScale);
    /* rescale to _lab_side x _lab_side */
    xf = xf * _lab_side;
    zf = zf * _lab_side;

    if((int)xf != xi || (int)zf != zi) {
      xi = (int)xf;
      zi = (int)zf;
    } 

    if(xi >= 0 && xi < _lab_side && zi >= 0 && zi < _lab_side && _labyrinth[zi * _lab_side + xi] == -1)
      _collision_dir[i] = 1;
    else
      _collision_dir[i] = 0;
  }

  for(i = 0; i < 4; i++) {
    if(_collision_dir[i]) {
      // printf("i : %d\n", i);
      col = 1;
    }
  }


  return col;
}

/*!\brief function called by GL4Dummies' loop at idle.
 * 
 * uses the virtual keyboard states to move the camera according to
 * direction, orientation and time (dt = delta-time)
 */
static void idle(void) {
  double dt, dtheta = M_PI, step = 15.0;
  static double t0 = 0, t;
  dt = ((t = gl4dGetElapsedTime()) - t0) / 1000.0;
  t0 = t;
  if(_keys[KLEFT])
    _cam.theta += dt * dtheta;
  if(_keys[KRIGHT])
    _cam.theta -= dt * dtheta;
  if(_keys[KUP]) {
    _cam.x += -dt * step * sin(_cam.theta);
    _cam.z += -dt * step * cos(_cam.theta);
    if(collision()) {
      if(_collision_dir[KLEFT] || _collision_dir[KRIGHT])
        _cam.x -= -dt * step * sin(_cam.theta);
      if(_collision_dir[KUP] || _collision_dir[KDOWN])
        _cam.z -= -dt * step * cos(_cam.theta);
    }
  }
  if(_keys[KDOWN]) {
    _cam.x += dt * step * sin(_cam.theta);
    _cam.z += dt * step * cos(_cam.theta);
    if(collision()) {
      if(_collision_dir[KLEFT] || _collision_dir[KRIGHT])
        _cam.x -= dt * step * sin(_cam.theta);
      if(_collision_dir[KUP] || _collision_dir[KDOWN])
        _cam.z -= dt * step * cos(_cam.theta);
    }
  }
  updatePosition();
}

/*!\brief function called by GL4Dummies' loop at key-down (key
 * pressed) event.
 * 
 * stores the virtual keyboard states (1 = pressed) and toggles the
 * boolean parameters of the application.
 */
static void keydown(int keycode) {
  GLint v[2];
  switch(keycode) {
  case GL4DK_LEFT:
    _keys[KLEFT] = 1;
    break;
  case GL4DK_RIGHT:
    _keys[KRIGHT] = 1;
    break;
  case GL4DK_UP:
    _keys[KUP] = 1;
    break;
  case GL4DK_DOWN:
    _keys[KDOWN] = 1;
    break;
  case GL4DK_ESCAPE:
  case 'q':
    exit(0);
    /* when 'w' pressed, toggle between line and filled mode */
  case 'w':
    glGetIntegerv(GL_POLYGON_MODE, v);
    if(v[0] == GL_FILL) {
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
      glLineWidth(3.0);
    } else {
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
      glLineWidth(1.0);
    }
    break;
    /* when 'm' pressed, toggle between mipmapping or nearest for the plane texture */
  case 'm': {
    _mipmap = !_mipmap;
    glBindTexture(GL_TEXTURE_2D, _planeTexId);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, _mipmap ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, _mipmap ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
    break;
  }
    /* when 'a' pressed, toggle on/off the anisotropic mode */
  case 'a': {
    _anisotropic = !_anisotropic;
    /* l'Anisotropic sous GL ne fonctionne que si la version de la
       bibliothèque le supporte ; supprimer le bloc ci-après si
       problème à la compilation. */
#ifdef GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT
    GLfloat max;
    glBindTexture(GL_TEXTURE_2D, _planeTexId);
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &max);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, _anisotropic ? max : 1.0f);
    glBindTexture(GL_TEXTURE_2D, 0);
#endif
    break;
  }
  default:
    break;
  }
}

/*!\brief function called by GL4Dummies' loop at key-up (key
 * released) event.
 * 
 * stores the virtual keyboard states (0 = released).
 */
static void keyup(int keycode) {
  switch(keycode) {
  case GL4DK_LEFT:
    _keys[KLEFT] = 0;
    break;
  case GL4DK_RIGHT:
    _keys[KRIGHT] = 0;
    break;
  case GL4DK_UP:
    _keys[KUP] = 0;
    break;
  case GL4DK_DOWN:
    _keys[KDOWN] = 0;
    break;
  default:
    break;
  }
}

/*!\brief function called by GL4Dummies' loop at the passive mouse motion event.*/
static void pmotion(int x, int y) {
  _xm = x; 
  _ym = y;
}

/*!\brief function called by GL4Dummies' loop at draw.*/
static void draw(void) {
  /* clears the OpenGL color buffer and depth buffer */
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  /* sets the current program shader to _pId */
  glUseProgram(_pId);
  gl4duBindMatrix("viewMatrix");
  /* loads the identity matrix in the current GL4Dummies matrix ("viewMatrix") */
  gl4duLoadIdentityf();
  /* modifies the current matrix to simulate camera position and orientation in the scene */
  /* see gl4duLookAtf documentation or gluLookAt documentation */
  gl4duLookAtf(_cam.x, 3.0, _cam.z, 
         _cam.x - sin(_cam.theta), 3.0 - (_ym - (_wH >> 1)) / (GLfloat)_wH, _cam.z - cos(_cam.theta), 
         0.0, 1.0,0.0);
  gl4duBindMatrix("modelMatrix");
  /* loads the identity matrix in the current GL4Dummies matrix ("modelMatrix") */
  gl4duLoadIdentityf();
  /* sets the current texture stage to 0 */
  glActiveTexture(GL_TEXTURE0);
  /* tells the pId program that "tex" is set to stage 0 */
  glUniform1i(glGetUniformLocation(_pId, "tex"), 0);

  /* pushs (saves) the current matrix (modelMatrix), scales, rotates,
   * sends matrices to pId and then pops (restore) the matrix */
  gl4duPushMatrix(); {
    gl4duRotatef(-90, 1, 0, 0);
    gl4duScalef(_planeScale, _planeScale, 1);
    gl4duSendMatrices();
  } gl4duPopMatrix();
  /* culls the back faces */
  glCullFace(GL_BACK);
  /* uses the checkboard texture */
  glBindTexture(GL_TEXTURE_2D, _planeTexId);
  /* sets in pId the uniform variable texRepeat to the plane scale */
  glUniform1f(glGetUniformLocation(_pId, "texRepeat"), 1.0);
  /* draws the plane */
  gl4dgDraw(_plane);

  int i, j;
  GLfloat sc = _planeScale/_lab_side;
  GLfloat res = _planeScale/_lab_side * 2.0f;
  for(i = 0; i < _lab_side; i++) {
    for(j = 0; j < _lab_side; j++) {
      if(_labyrinth[(_lab_side - 1 - j) * _lab_side + i] == -1) {
        int xi = i - _lab_side/2, zi = j - _lab_side/2;
        gl4duPushMatrix(); {
          gl4duTranslatef(xi * res, sc, zi * res);
          gl4duScalef(sc, sc, sc);
          gl4duSendMatrices();
        } gl4duPopMatrix();

        glCullFace(GL_BACK);
        glBindTexture(GL_TEXTURE_2D, _wallTexId);
        gl4dgDraw(_cube);
      }
    }
  }

  GLfloat xf = _item.x - _lab_side/2.0f, zf = _item.z - _lab_side/2.0;
  // printf("item :%f - %f\n\n", _item.x, _item.z);
  gl4duPushMatrix(); {
    gl4duTranslatef(xf * _planeScale/_lab_side * 2.0f, 1, -zf * _planeScale/_lab_side * 2.0f);
    gl4duScalef(_item.w, 1, _item.h);
    gl4duSendMatrices();
  } gl4duPopMatrix();
  glCullFace(GL_BACK);
  glBindTexture(GL_TEXTURE_2D, _itemTexId);
  gl4dgDraw(_cube);

  /* the compass should be drawn in an orthographic projection, thus
   * we should bind the projection matrix; save it; load identity;
   * bind the model-view matrix; modify it to place the compass at the
   * top left of the screen and rotate the compass according to the
   * camera orientation (theta); send matrices; restore the model-view
   * matrix; bind the projection matrix and restore it; and then
   * re-bind the model-view matrix for after. */
  gl4duBindMatrix("projectionMatrix");
  gl4duPushMatrix(); {
    gl4duLoadIdentityf();
    gl4duBindMatrix("modelMatrix");
    gl4duPushMatrix(); {
      gl4duLoadIdentityf();
      gl4duTranslatef(-0.75, 0.7, 0.0);
      gl4duRotatef(-_cam.theta * 180.0 / M_PI, 0, 0, 1);
      gl4duScalef(0.03 / 5.0, 1.0 / 5.0, 1.0 / 5.0);
      gl4duBindMatrix("viewMatrix");
      gl4duPushMatrix(); {
  gl4duLoadIdentityf();
  gl4duSendMatrices();
      } gl4duPopMatrix();
      gl4duBindMatrix("modelMatrix");
    } gl4duPopMatrix();
    gl4duBindMatrix("projectionMatrix");
  } gl4duPopMatrix();
  gl4duBindMatrix("modelMatrix");
  /* disables cull facing and depth testing */
  glDisable(GL_CULL_FACE);
  glDisable(GL_DEPTH_TEST);
  /* uses the compass texture */
  glBindTexture(GL_TEXTURE_2D, _compassTexId);
  /* texture repeat only once */
  glUniform1f(glGetUniformLocation(_pId, "texRepeat"), 1);
  /* draws the compass */
  gl4dgDraw(_cube);

  gl4duBindMatrix("projectionMatrix");
  gl4duPushMatrix(); {
    gl4duLoadIdentityf();
    gl4duOrthof(-1.0, 1.0, -_wH / (GLfloat)_wW, _wH / (GLfloat)_wW, 0.0, 2.0);
    gl4duBindMatrix("modelMatrix");
    gl4duPushMatrix(); {
      gl4duLoadIdentityf();
      gl4duTranslatef(0.75, -0.4, 0.0);
      gl4duRotatef(-_cam.theta * 180.0 / M_PI, 0, 0, 1);
      gl4duScalef(1.0 / 5.0, 1.0 / 5.0, 1.0);
      gl4duBindMatrix("viewMatrix");
      gl4duPushMatrix(); {
  gl4duLoadIdentityf();
  gl4duSendMatrices();
      } gl4duPopMatrix();
      gl4duBindMatrix("modelMatrix");
    } gl4duPopMatrix();
    gl4duBindMatrix("projectionMatrix");
  } gl4duPopMatrix();
  gl4duBindMatrix("modelMatrix");
  /* disables cull facing and depth testing */
  glDisable(GL_CULL_FACE);
  glDisable(GL_DEPTH_TEST);
  /* uses the labyrinth texture */
  glBindTexture(GL_TEXTURE_2D, _planeTexId);

  /* draws borders */
  glUniform1i(glGetUniformLocation(_pId, "border"), 1);
  /* draws the map */
  gl4dgDraw(_cube);
  /* do not draw borders */
  glUniform1i(glGetUniformLocation(_pId, "border"), 0);

  /* enables cull facing and depth testing */
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
}

/*!\brief function called at exit. Frees used textures and clean-up
 * GL4Dummies.*/
static void quit(void) {
  if(_labyrinth) 
    free(_labyrinth);    
  if(_planeTexId)
    glDeleteTextures(1, &_planeTexId);
  if(_compassTexId)
    glDeleteTextures(1, &_compassTexId);
  gl4duClean(GL4DU_ALL);
}
