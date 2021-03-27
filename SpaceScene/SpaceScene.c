/**********************************************************
* Solar System OpenGL ES2
* Description: Solar system model
* Build command:  gcc SpaceScene.c ../lib/glmath.c -lGLESv2 -lglfw -lm -Wall
* References:
**********************************************************/

/*******************************************************************/
/*  Includes                                                       */
/*******************************************************************/
#define GLFW_INCLUDE_ES2
#include <GLFW/glfw3.h>
#include <GLES3/gl3.h>
#include <GLES2/gl2ext.h>

#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <termios.h>

#include "../lib/glmath.h"

/*******************************************************************/
/*  Defines                                                        */
/*******************************************************************/
#define PI                          3.141592654f

#define DISPLAY_WIDTH               1024.0f
#define DISPLAY_HEIGHT              768.0f

#define BMP_WIDTH_OFFSET            18
#define BMP_HEIGHT_OFFSET           22
#define BMP_PIX_PER_COL             3
#define BMP_HEADER_SIZE             54

#define DIFFUSE_LIMIT               5.28f

#define SCENE_NEAR                  0.1f
#define SCENE_FAR                   20000.0f
#define SCENE_RIGHT                 DISPLAY_WIDTH
#define SCENE_TOP                   DISPLAY_HEIGHT
#define SCENE_LEFT                  0.0f
#define SCENE_BOTTOM                0.0f

#define MOVE                        20.0f
#define MOVE_BIG                    100.0f

#define DEFAULT_FOV                 35.0f

#define KEYS_UP                     65
#define KEYS_DOWN                   'B'
#define KEYS_RIGHT                  'C'
#define KEYS_LEFT                   'D'
#define KEYS_PGDN                   '6'
#define KEYS_PGUP                   '5'
#define KEYS_HOME                   72
#define KEYS_END                    'F'
#define KEYS_ESC                    '`'

#define UNIVERSE                    "textures/universe.bmp"
#define STAR_SUN                    "textures/star_Sun.bmp"
#define PLANET_EARTH                "textures/planet_Earth.bmp"
#define PLANET_MOON                 "textures/planet_Moon.bmp"
#define PLANET_JUPITER              "textures/planet_Jupiter.bmp"
#define PLANET_MARS                 "textures/planet_Mars.bmp"
#define PLANET_MERCURY              "textures/planet_Mercury.bmp"
#define PLANET_NEPTUNE              "textures/planet_Neptune.bmp"
#define PLANET_PLUTO                "textures/planet_Pluto.bmp"
#define PLANET_SATURN               "textures/planet_Saturn.bmp"
#define RINGS_SATURN_COLOR          "textures/rings_SaturnColor.bmp"
#define RINGS_SATURN_MASK           "textures/rings_SaturnMask.bmp"
#define PLANET_URANUS               "textures/planet_Uranus.bmp"
#define RINGS_URANUS_COLOR          "textures/rings_UranusColor.bmp"
#define RINGS_URANUS_MASK           "textures/rings_UranusMask.bmp"
#define PLANET_VENUS                "textures/planet_Venus.bmp"


/*******************************************************************/
/*  Structures                                                     */
/*******************************************************************/
typedef struct _textureData_t
{
    const char *fileName;
    GLuint id;
    GLsizei width;
    GLsizei height;
} textureData_t;

typedef struct _modelData_t
{
    GLint numOfVerts;
    GLfloat *verts;
    GLfloat *texCoords;
} modelData_t;

typedef struct _rings_t
{
    textureData_t texColor;
    textureData_t texMask;
    modelData_t model;
    GLfloat planetGap;
    GLfloat outerRadius;
    GLint numOfSegments;
} rings_t;

typedef struct _sphere_t
{
    modelData_t model;
    GLfloat radius;
    GLfloat gradation;
} sphere_t;

typedef struct _celestial_t
{
    const char *name;
    textureData_t texColor;
    sphere_t sphere;
    vec3_t origin;
    vec3_t currentPosition;
    vec3_t rotAxisPlanet;
    GLfloat rotSpeedPlanet;
    GLfloat rotAnglePlanet;
    vec3_t rotAxisOrbit;
    GLfloat rotSpeedOrbit;
    GLfloat rotAngleOrbit;
    GLfloat initRotAngle;
    vec3_t initRotAxis;
    vec3_t scale;
    GLfloat diffuseValue;
    rings_t rings;
} celestial_t;


/*******************************************************************/
/*  Enums                                                          */
/*******************************************************************/
typedef enum _retCode_e {
    RET_FAIL = -1,
    RET_SUCCESS,
} retCode_e;


/*******************************************************************/
/*  Globals                                                        */
/*******************************************************************/
static celestial_t celestialObject[] = {
    {
        "Universe",
        {UNIVERSE, 0, 0, 0},
        { {0, NULL, NULL}, 10000.0f, 50.0f},
        {0.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f},
        0.0f,
        0.0f,
        {0.0f, 1.0f, 0.0f},
        0.0f,
        0.0f,
        0.0f,
        {0.0f, 0.0f, 0.0f},
        {1.0f, 1.0f, 1.0f},
        0.70f,
        { {NULL, 0, 0, 0}, {NULL, 0, 0, 0}, {0, NULL, NULL}, 0.0f, 0.0f, 0 },
    },

    {
        "Sun",
        {STAR_SUN, 0, 0, 0},
        { {0, NULL, NULL}, 500.0f, 50.0f},
        {0.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f},
        0.05f,
        0.0f,
        {0.0f, 1.0f, 0.0f},
        0.0f,
        0.0f,
        90.0f,
        {1.0f, 0.0f, 0.0f},
        {1.0f, 1.0f, 1.0f},
        0.95f,
        { {NULL, 0, 0, 0}, {NULL, 0, 0, 0}, {0, NULL, NULL}, 0.0f, 0.0f, 0 },
    },

    {
        "Mercury",
        {PLANET_MERCURY, 0, 0, 0},
        { {0, NULL, NULL}, 15.16f, 50.0f},
        {600.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f},
        0.21f,
        0.0f,
        {0.0f, 1.0f, 0.3f},
        0.1f,
        0.0f,
        90.0f,
        {1.0f, 0.0f, 0.0f},
        {1.0f, 1.0f, 1.0f},
        0.0f,
        { {NULL, 0, 0, 0}, {NULL, 0, 0, 0}, {0, NULL, NULL}, 0.0f, 0.0f, 0 },
    },

    {
        "Venus",
        {PLANET_VENUS, 0, 0, 0},
        { {0, NULL, NULL}, 37.60f, 50.0f},
        {800.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f},
        0.41f,
        0.0f,
        {0.0f, 1.0f, 0.0f},
        0.2f,
        0.0f,
        90.0f,
        {1.0f, 0.0f, 0.0f},
        {1.0f, 1.0f, 1.0f},
        0.0f,
        { {NULL, 0, 0, 0}, {NULL, 0, 0, 0}, {0, NULL, NULL}, 0.0f, 0.0f, 0 },
    },

    {
        "Earth",
        {PLANET_EARTH, 0, 0, 0},
        { {0, NULL, NULL}, 39.59f, 50.0f},
        {1000.0f, 0.0f, 200.0f},
        {0.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f},
        1.15f,
        0.0f,
        {0.0f, 1.0f, 0.0f},
        0.30f,
        0.0f,
        90.0f,
        {1.0f, 0.0f, 0.0f},
        {1.0f, 1.0f, 1.0f},
        0.0f,
        { {NULL, 0, 0, 0}, {NULL, 0, 0, 0}, {0, NULL, NULL}, 0.0f, 0.0f, 0 },
    },

    {
        "Moon",
        {PLANET_MOON, 0, 0, 0},
        { {0, NULL, NULL}, 15.0f, 50.0f},
        {1020.0f, 100.0f, 100.0f},
        {0.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f},
        0.0f,
        0.0f,
        {0.0f, 1.0f, 0.0f},
        0.3f,
        0.0f,
        90.0f,
        {1.0f, 0.0f, 0.0f},
        {1.0f, 1.0f, 1.0f},
        0.0f,
        { {NULL, 0, 0, 0}, {NULL, 0, 0, 0}, {0, NULL, NULL}, 0.0f, 0.0f, 0 },
    },

    {
        "Mars",
        {PLANET_MARS, 0, 0, 0},
        { {0, NULL, NULL}, 33.96f, 50.0f},
        {1200.0f, 300.0f, 0.0f},
        {0.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f},
        0.31f,
        0.0f,
        {0.0f, 1.0f, 0.0f},
        0.21f,
        0.0f,
        90.0f,
        {1.0f, 0.0f, 0.0f},
        {1.0f, 1.0f, 1.0f},
        0.0f,
        { {NULL, 0, 0, 0}, {NULL, 0, 0, 0}, {0, NULL, NULL}, 0.0f, 0.0f, 0 },
    },

    {
        "Jupiter",
        {PLANET_JUPITER, 0, 0, 0},
        { {0, NULL, NULL}, 43.441f, 50.0f},
        {1400.0f, 200.0f, 0.0f},
        {0.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f},
        1.21f,
        0.0f,
        {0.0f, 1.0f, 0.0f},
        0.12f,
        0.0f,
        90.0f,
        {1.0f, 0.0f, 0.0f},
        {1.0f, 1.0f, 1.0f},
        0.0f,
        { {NULL, 0, 0, 0}, {NULL, 0, 0, 0}, {0, NULL, NULL}, 0.0f, 0.0f, 0 },
    },

    {
        "Saturn",
        {PLANET_SATURN, 0, 0, 0},
        { {0, NULL, NULL}, 36.184f, 50.0f},
        {1600.0f, 150.0f, 0.0f},
        {0.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f},
        1.21f,
        27.0f,
        {0.0f, 1.0f, 0.0f},
        0.42f,
        0.0f,
        90.0f,
        {1.0f, 0.0f, 0.0f},
        {1.0f, 1.0f, 1.0f},
        0.0f,
        { {RINGS_SATURN_COLOR, 0, 0, 0}, {RINGS_SATURN_MASK, 0, 0, 0}, {0, NULL, NULL}, 8.0f, 100.0f, 720 },
    },

    {
        "Uranus",
        {PLANET_URANUS, 0, 0, 0},
        { {0, NULL, NULL}, 15.75f, 50.0f},
        {1800.0f, 110.0f, 0.0f},
        {0.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 0.0f},
        1.21f,
        0.0f,
        {0.0f, 1.0f, 0.0f},
        0.52f,
        0.0f,
        90.0f,
        {0.0f, 1.0f, 0.0f},
        {1.0f, 1.0f, 1.0f},
        0.0f,
        { {RINGS_URANUS_COLOR, 0, 0, 0}, {RINGS_URANUS_MASK, 0, 0, 0}, {0, NULL, NULL}, 8.0f, 30.0f, 720 },
    },

    {
        "Neptune",
        {PLANET_NEPTUNE, 0, 0, 0},
        { {0, NULL, NULL}, 15.299f, 50.0f},
        {2000.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f},
        1.21f,
        0.0f,
        {0.0f, 1.0f, 0.0f},
        0.32f,
        0.0f,
        90.0f,
        {1.0f, 0.0f, 0.0f},
        {1.0f, 1.0f, 1.0f},
        0.0f,
        { {NULL, 0, 0, 0}, {NULL, 0, 0, 0}, {0, NULL, NULL}, 0.0f, 0.0f, 0 },
    },

    {
        "Pluto",
        {PLANET_PLUTO, 0, 0, 0},
        { {0, NULL, NULL}, 9.299f, 50.0f},
        {2200.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f},
        1.41f,
        0.0f,
        {0.0f, 1.0f, 0.0f},
        0.42f,
        0.0f,
        90.0f,
        {1.0f, 0.0f, 0.0f},
        {1.0f, 1.0f, 1.0f},
        0.0f,
        { {NULL, 0, 0, 0}, {NULL, 0, 0, 0}, {0, NULL, NULL}, 0.0f, 0.0f, 0 },
    },

};

static const GLchar* vertex_shader_source =    
{
    "precision mediump float;\n"

    "attribute vec2 aTexCoords;\n"
    "attribute vec4 aPosition;\n"

    "varying vec2 vTexCoords;\n"
    "varying vec4 vNormal;\n"
    "varying vec4 vPosition;\n"

    "uniform mat4 uMVP;\n"
    "uniform mat4 uRotateMatrix;\n"

    "void main(void)\n"
    "{\n"
        "vTexCoords = aTexCoords;\n"
        "vPosition = vec4(aPosition.xyz, 1.0) * uRotateMatrix;\n"
        "vNormal = vec4(aPosition.xyz, 0.0) * uRotateMatrix;\n"

        "gl_Position = vPosition * uMVP;\n"
    "}\n"
};

static const GLchar* fragment_shader_source =
{
    "precision mediump float;\n"

    "varying vec2 vTexCoords;\n"
    "varying vec4 vPosition;\n"
    "varying vec4 vNormal;\n"

    "uniform sampler2D uTextureColor;\n"

    "uniform sampler2D uTextureMask;\n"
    "uniform bool uBlendTextures;\n"
    "uniform vec4 uLightPos;\n"
    "uniform float uDiffuseValue;\n"
    "uniform float uDiffuseLimit;\n"

    "void main(void)\n"
    "{\n"   
        "float distance = length(uLightPos.xyz - vPosition.xyz);\n"
        "vec3 lightVector = normalize(uLightPos.xyz - vPosition.xyz);\n"

        "float diffuse = max(dot(vNormal.xyz, lightVector.xyz), 0.1);\n"
        "diffuse = diffuse * (10000.0 / (1.0 + (0.25 * (distance * distance))));\n"
        "diffuse = (uDiffuseValue > 0.0) ? uDiffuseValue : diffuse;\n"
        
        "vec4 color = texture2D(uTextureColor, vTexCoords);\n"
        "vec4 mask = texture2D(uTextureMask, vTexCoords);\n"
        
        "gl_FragColor = (uBlendTextures == true) ? vec4((color.bgr * mask.bgr), 0.3) : gl_FragColor = vec4(color.bgr, 1.0) * diffuse;\n"
    "}\n"
};


static GLfloat persepctiveProjMatrix[16] = {0.0f};
static GLfloat modelViewProjMatrix[16]   = {0.0f};
static GLfloat rotationMatrixUp[16]      = {0};
static GLfloat rotationMatrixRight[16]   = {0};
static GLfloat modelRotationUp           = 50.0f;
static GLfloat modelRotationRight        = -19.0f;

static vec3_t cameraRight                = {0.0f, 0.0f, 0.0f};
static vec3_t cameraDirection            = {0.0f, 0.0f, 0.0f};
static vec3_t cameraTarget               = {0.0f, 0.0f, 0.0f};

static vec3_t cameraPosition             = {-840.0f, 0.0f, 3620.0f};
static vec3_t cameraFront                = {0.0f, 0.0f, -1.0f};
static vec3_t cameraUp                   = {0.0f, 1.0f,  0.0f};

static vec4_t lightPosition              = {0.0f, 0.0f, 0.0f, 0.0f};

static GLint shaderProgram               = -1;
static GLint aVertexLoc                  = -1;
static GLint aTexCoordsLoc               = -1;

static GLboolean uBlendTexturesLoc       = -1;
static GLint uTextureColorLoc            = -1;
static GLint uTextureMaskLoc             = -1;
static GLint uLightPosLoc                = -1;
static GLint uMVPLoc                     = -1;
static GLint uDiffuseValueLoc            = -1;
static GLint uDiffuseLimitLoc            = -1;
static GLint uRotateMatrixLoc            = -1;

static GLint appShutdown                 = 0;


/*******************************************************************/
/*  Functions                                                      */
/*******************************************************************/
int kbhit (void)
{
    struct termios save_termios;
    struct termios ios;
    fd_set inp;
    struct timeval timeout = {0, 0};
    int result;

    if (!isatty (STDIN_FILENO))
    return 0;

    if (tcgetattr (STDIN_FILENO, &save_termios) < 0)
    return 0;

    ios = save_termios;
    ios.c_lflag &= ~(ICANON | ECHO | ISIG);
    ios.c_cc[VMIN] = 1;           /* read() will return with one char */
    ios.c_cc[VTIME] = 0;          /* read() blocks forever */

    if (tcsetattr (STDIN_FILENO, TCSANOW, &ios) < 0)
    return 0;

    /* set up select() args */
    FD_ZERO(&inp);
    FD_SET(STDIN_FILENO, &inp);

    result = select (STDIN_FILENO+1, &inp, NULL, NULL, &timeout) == 1;

    tcsetattr (STDIN_FILENO, TCSANOW, &save_termios);

    return result;
}


char getch(void)
{
    char buf = 0;
    struct termios old = {0};
    fflush(stdout);
    if(tcgetattr(0, &old) < 0)
        perror("tcsetattr()");
    old.c_lflag &= ~ICANON;
    old.c_lflag &= ~ECHO;
    old.c_cc[VMIN] = 1;
    old.c_cc[VTIME] = 0;
    if(tcsetattr(0, TCSANOW, &old) < 0)
        perror("tcsetattr ICANON");
    if(read(0, &buf, 1) < 0)
        perror("read()");
    old.c_lflag |= ICANON;
    old.c_lflag |= ECHO;
    if(tcsetattr(0, TCSADRAIN, &old) < 0)
        perror("tcsetattr ~ICANON");
    return buf;
}


GLint loadShaderProgram(const char *vertex_shader_source, const char *fragment_shader_source) {
    enum Consts {INFOLOG_LEN = 512};
    GLchar infoLog[INFOLOG_LEN];
    GLint fragment_shader;
    GLint shader_program;
    GLint success;
    GLint vertex_shader;

    /* Vertex shader */
    vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_shader_source, NULL);
    glCompileShader(vertex_shader);
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertex_shader, INFOLOG_LEN, NULL, infoLog);
        printf("ERROR::SHADER::VERTEX::COMPILATION_FAILED\n%s\n", infoLog);
    }

    /* Fragment shader */
    fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragment_shader_source, NULL);
    glCompileShader(fragment_shader);
    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragment_shader, INFOLOG_LEN, NULL, infoLog);
        printf("ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n%s\n", infoLog);
    }

    /* Link shaders */
    shader_program = glCreateProgram();
    glAttachShader(shader_program, vertex_shader);
    glAttachShader(shader_program, fragment_shader);
    glLinkProgram(shader_program);
    glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shader_program, INFOLOG_LEN, NULL, infoLog);
        printf("ERROR::SHADER::PROGRAM::LINKING_FAILED\n%s\n", infoLog);

    }

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    return shader_program;
}


void createRings(celestial_t *body)
{
    GLint vertsIncSize = ( sizeof(GLfloat) * 4 * 2);
    GLint texsIncSize = ( sizeof(GLfloat) * 2 * 2);
    GLint vertsTotalSize = vertsIncSize;
    GLint texsTotalSize = texsIncSize;
    GLint vOff = 0;
    GLint tOff = 0;
    GLint seg;

    GLfloat **verts = &body->rings.model.verts;
    GLfloat **texCoords = &body->rings.model.texCoords;

    *verts = (GLfloat*)malloc(vertsIncSize);
    *texCoords = (GLfloat*)malloc(texsIncSize);

    for(seg=0;seg<body->rings.numOfSegments+1;seg++)
    {
        GLfloat theta = 360.0-(2.0f * PI * (GLfloat)seg/(GLfloat)body->rings.numOfSegments);

        *(*verts+vOff++) = (body->sphere.radius + body->rings.planetGap) * cosf(theta);
        *(*verts+vOff++) = (body->sphere.radius + body->rings.planetGap) * sinf(theta);
        *(*verts+vOff++) = 0.0f;
        *(*verts+vOff++) = 1.0f;

        *(*verts+vOff++) = body->rings.outerRadius * cosf(theta);
        *(*verts+vOff++) = body->rings.outerRadius * sinf(theta);
        *(*verts+vOff++) = 0.0f;
        *(*verts+vOff++) = 1.0f;

        *(*texCoords+tOff++) = 0.0f;
        *(*texCoords+tOff++) = 0.0f;

        *(*texCoords+tOff++) = 1.0f;
        *(*texCoords+tOff++) = 1.0f;

        vertsTotalSize += vertsIncSize;
        texsTotalSize += texsIncSize;

        *verts = realloc(*verts, vertsTotalSize);
        *texCoords = realloc(*texCoords, texsTotalSize);
    }

    /* Store the number of vertices */
    body->rings.model.numOfVerts = (vertsTotalSize/(vertsIncSize/2))-2;
}


void createSphere(celestial_t *body)
{
    GLint vertArrayInc = (sizeof(float) * 4 * 2);
    GLint texArrayInc = (sizeof(float) * 2 * 2);
    GLint vertArraySize = vertArrayInc;
    GLint texArraySize = texArrayInc;
    GLint vOff = 0;
    GLint tOff = 0;
    GLfloat alpha, beta;

    GLfloat radius = body->sphere.radius;
    GLfloat gradation = body->sphere.gradation;
    GLfloat **vertices = &body->sphere.model.verts;
    GLfloat **texCoords = &body->sphere.model.texCoords;

    *vertices = (GLfloat*)(malloc(vertArraySize));
    *texCoords = (GLfloat*)(malloc(texArraySize));

    for (alpha = 0.0f; alpha < 2.001f * PI-PI/gradation; alpha += PI / gradation)
    {
        for (beta = 0.0f; beta < 2.001f * PI; beta += PI / gradation)
        {
            *(*vertices+vOff++) = (radius * cosf(beta) * sinf(alpha));
            *(*vertices+vOff++) = (radius * sinf(beta) * sinf(alpha));
            *(*vertices+vOff++) = (radius * cosf(alpha));
            *(*vertices+vOff++) = 0.0f;

            *(*vertices+vOff++) = (radius * cosf(beta) * sinf(alpha + PI / gradation));
            *(*vertices+vOff++) = (radius * sinf(beta) * sinf(alpha + PI / gradation));
            *(*vertices+vOff++) = (radius * cosf(alpha + PI / gradation));
            *(*vertices+vOff++) = 0.0f;

            *(*texCoords+tOff++) = beta / (2.0f * PI);
            *(*texCoords+tOff++) = alpha / PI;

            *(*texCoords+tOff++) = beta / (2.0f * PI);
            *(*texCoords+tOff++) = alpha / PI + 1.0f / gradation;

            vertArraySize += vertArrayInc;
            texArraySize += texArrayInc;

            *vertices = (GLfloat*)(realloc(*vertices, vertArraySize));
            *texCoords = (GLfloat*)(realloc(*texCoords, texArraySize));
        }
    }

    /* Store the number of vertices */
    body->sphere.model.numOfVerts = (vertArraySize/vertArrayInc)-1;
}


void loadShader(void)
{
    /* Load shader program */
    shaderProgram = loadShaderProgram(vertex_shader_source, fragment_shader_source);

    /* Get the vertex attribute and color uniform locations */
    aVertexLoc = glGetAttribLocation(shaderProgram, "aPosition");
    aTexCoordsLoc = glGetAttribLocation(shaderProgram, "aTexCoords");

    uTextureColorLoc = glGetUniformLocation(shaderProgram, "uTextureColor");
    uTextureMaskLoc = glGetUniformLocation(shaderProgram, "uTextureMask");
    uBlendTexturesLoc = glGetUniformLocation(shaderProgram, "uBlendTextures");
        
    uMVPLoc = glGetUniformLocation(shaderProgram, "uMVP");
    uLightPosLoc = glGetUniformLocation(shaderProgram, "uLightPos");
    uDiffuseValueLoc = glGetUniformLocation(shaderProgram, "uDiffuseValue");
    uRotateMatrixLoc = glGetUniformLocation(shaderProgram, "uRotateMatrix");
    uDiffuseLimitLoc = glGetUniformLocation(shaderProgram, "uDiffuseLimit");

    /* Use program */
    glUseProgram(shaderProgram);

    /* Bind uniform samplers to texture units */
    glUniform1i(uTextureColorLoc, 0);
    glUniform1i(uTextureMaskLoc, 1);
}


void drawcelestialObject(celestial_t *body)
{
    GLfloat rotationPlanet[16]          = {0.0f};
    GLfloat rotationOrbit[16]           = {0.0f};
    GLfloat scaleTranslationMatrix[16]  = {0.0f};

    /* Calculate planet rotation */
    body->rotAnglePlanet += body->rotSpeedPlanet;
    if(body->rotAnglePlanet > 360.0f)
    {
        body->rotAnglePlanet = body->rotAnglePlanet-360.0f;
    }

    /* Calculate orbit rotation */
    body->rotAngleOrbit += body->rotSpeedOrbit;
    if(body->rotAngleOrbit > 360.0f)
    {
        body->rotAngleOrbit = body->rotAngleOrbit-360.0f;
    }

    /* Translate to origin */
    generateScaleTranslationMatrix(body->scale, body->origin, scaleTranslationMatrix);

    /* Rotate planet */
    generateRotationMatrix(body->rotAnglePlanet, body->rotAxisPlanet, rotationPlanet);
    matrix4x4By4x4(scaleTranslationMatrix, rotationPlanet, rotationPlanet);

    /* Rotate orbit */
    generateRotationMatrix(body->rotAngleOrbit, body->rotAxisOrbit, rotationOrbit);
    matrix4x4By4x4(rotationOrbit, rotationPlanet, rotationOrbit);

    /* Load the scale/rotation matrix */
    glUniformMatrix4fv(uRotateMatrixLoc, 1, 0, rotationOrbit);

    /* Override the diffuse value.  Used to light up the sun and universe */
    glUniform1f(uDiffuseValueLoc, body->diffuseValue);

    /* Bind the planet texture */
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, body->texColor.id);

    /* Disable texture blending */
    glUniform1i(uBlendTexturesLoc, GL_FALSE);

    /* Set up to draw */
    glVertexAttribPointer(aVertexLoc, 4, GL_FLOAT, GL_FALSE, 0, body->sphere.model.verts);
    glEnableVertexAttribArray(aVertexLoc);
    glVertexAttribPointer(aTexCoordsLoc, 2, GL_FLOAT, GL_FALSE, 0, body->sphere.model.texCoords);
    glEnableVertexAttribArray(aTexCoordsLoc);

    /* Draw sphere */
    glDrawArrays(GL_TRIANGLE_STRIP, 0, body->sphere.model.numOfVerts);

    glDisableVertexAttribArray(aVertexLoc);
    glDisableVertexAttribArray(aTexCoordsLoc);

    /* If the planet has rings */
    if ( body->rings.model.verts != NULL )
    {       
        /* Enable blending */
        glEnable(GL_BLEND);
   
           /* Bind the ring color and mask texturex */
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, body->rings.texColor.id);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, body->rings.texMask.id);

        /* Enable texture blending */
        glUniform1i(uBlendTexturesLoc, GL_TRUE);

        /* Set up vertex attrib pointers */
        glVertexAttribPointer(aVertexLoc, 4, GL_FLOAT, GL_FALSE, 0, body->rings.model.verts);
        glVertexAttribPointer(aTexCoordsLoc, 2, GL_FLOAT, GL_FALSE, 0, body->rings.model.texCoords);

        glEnableVertexAttribArray(aVertexLoc);
        glEnableVertexAttribArray(aTexCoordsLoc);

        /* Draw the rings */
        glDrawArrays(GL_TRIANGLE_STRIP, 0, body->rings.model.numOfVerts);
        
        glDisableVertexAttribArray(aVertexLoc);
        glDisableVertexAttribArray(aTexCoordsLoc);

        /* Disable blending */
        glDisable(GL_BLEND);
    }
}


retCode_e loadTexture(textureData_t *texStruct)
{
    GLubyte *data = NULL;
    FILE *file = NULL;

    /* Open File */
    file = fopen( texStruct->fileName, "rb" );
    if ( file == NULL ) 
    {
        return RET_FAIL;
    }

    /* Read file */
    data = (GLubyte *)malloc( BMP_HEADER_SIZE );

    /* Read BMP Header */
    fread(data, BMP_HEADER_SIZE, 1, file);

    /* Get width and height from header */
    texStruct->width = *(GLuint *)(&data[BMP_WIDTH_OFFSET]);
    texStruct->height = *(GLuint *)(&data[BMP_HEIGHT_OFFSET]);

    /* Read pixel data */
    data = (GLubyte *)realloc( data, texStruct->width * texStruct->height * 3 );
    fread( data, texStruct->width * texStruct->height * BMP_PIX_PER_COL, 1, file );
    fclose( file );

    /* Generate and store texture */
    glGenTextures(1, &texStruct->id);
    glBindTexture(GL_TEXTURE_2D, texStruct->id);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB8, texStruct->width, texStruct->height);
    glTexSubImage2D(GL_TEXTURE_2D, 0,0,0, texStruct->width, texStruct->height, GL_RGB, GL_UNSIGNED_BYTE, data);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,GL_NEAREST_MIPMAP_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,GL_NEAREST_MIPMAP_NEAREST);

    /* Free data */
    free(data);

    return RET_SUCCESS;
}


void createCelestialObjectObject(celestial_t *body)
{
    GLint i;
    GLfloat rotationMatrix[16] = {0.0};

    /* Load planet texture */
    if ( RET_FAIL == loadTexture(&body->texColor) )
    {
        printf("Error loading planet texture\n");
    }

    /* If this planet has rings */
    if ( body->rings.texColor.fileName != NULL )
    {
        printf("\tCreating rings\n");

        /* Load ring color */
        if ( RET_FAIL == loadTexture(&body->rings.texColor) )
        {
            printf("Error loading rings color texture\n");
        }

        /* Load ring mask */
        if ( RET_FAIL == loadTexture(&body->rings.texMask) )
        {
            printf("Error loading rings mask texture\n");
        }

        /* Create the rings */
        createRings(body);
    }

    /* Generate sphere */
    createSphere(body);

    /* Generate rotation matrix */
    generateRotationMatrix(body->initRotAngle, body->initRotAxis, (GLfloat*)&rotationMatrix);

    /* Apply rotation to sphere vertices */
    for(i=0; i< body->sphere.model.numOfVerts; i++)
    {
        matrix4x4By4x1(rotationMatrix, &body->sphere.model.verts[i*4], &body->sphere.model.verts[i*4]);
    }

    /* If this planet has rings */
    if ( body->rings.model.verts != NULL )
    {
        for(i=0; i<body->rings.model.numOfVerts; i++)
        {
            matrix4x4By4x1(rotationMatrix, &body->rings.model.verts[i*4], &body->rings.model.verts[i*4]);
        }
    }

    /* Set current position */
    body->currentPosition = body->origin;
}


void cleanUpcelestialObject(celestial_t *body)
{
    /* Free sphere texture data */
    if ( body->sphere.model.verts != NULL )
    {
        free(body->sphere.model.verts);
        free(body->sphere.model.texCoords);
    }

    /* Free rings texture data */
    if ( body->rings.model.verts != NULL )
    {
        free(body->rings.model.verts);
        free(body->rings.model.texCoords);
    }
}


void updateModelViewProjMatrix(void)
{
    GLfloat viewMatrix[16] = {0.0f};

    /* Generate lookAt matrix for camera */
    generateLookAtMatrix(cameraPosition,
                    (vec3_t){cameraPosition.x+cameraFront.x, cameraPosition.y+cameraFront.y, cameraPosition.z+cameraFront.z},
                    cameraUp, viewMatrix);

    /* Generate the rotation matrix */
    generateRotationMatrix(modelRotationUp, cameraUp, rotationMatrixUp);
    generateRotationMatrix(modelRotationRight, cameraRight, rotationMatrixRight);

    /* Apply the model rotation */
    matrix4x4By4x4(rotationMatrixUp, rotationMatrixRight, rotationMatrixRight);
    matrix4x4By4x4(viewMatrix, rotationMatrixRight, viewMatrix);

    /* Set the modelViewMatrix */
    matrix4x4By4x4(persepctiveProjMatrix, viewMatrix, modelViewProjMatrix);

    /* Load modelview matrix */
    glUniformMatrix4fv(uMVPLoc, 1, 0, modelViewProjMatrix);
}


void initGL(void)
{
    /* Set Aspect ratio */
    GLfloat aspect = (GLfloat)DISPLAY_WIDTH / (GLfloat)DISPLAY_HEIGHT;

    /* Set blend function for rings */
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    /* Enable depth test */
    glEnable(GL_DEPTH_TEST);
    
    /* Set depth function to less than equal */
    glDepthFunc(GL_LEQUAL);
    
    /* Set depth range */
    glDepthRangef(0.0f, 1.0f);

    /* Set clear color */
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    /* Set viewport */
    glViewport(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT);

    /* Generate perspective/projection matrix */
    generatePerspectiveProjectionMatrix(DEFAULT_FOV, aspect, SCENE_NEAR, SCENE_FAR, persepctiveProjMatrix);

    /* Set inital camera and model settings */
    cameraDirection = normalize(subProd(cameraPosition, cameraTarget));
    cameraRight = normalize(crossProd(cameraUp, cameraDirection));
    cameraUp = crossProd(cameraDirection, cameraRight);

    /* Update the MVP matrix */
    updateModelViewProjMatrix();

    /* Load sun's light source */
    glUniform4fv(uLightPosLoc, 1, (GLfloat*)&lightPosition);
    
    /* Set the diffuse limit */
    glUniform1f(uDiffuseLimitLoc, DIFFUSE_LIMIT);
}


void checkUserInput(void)
{
    if(kbhit())
    {
        char c = getch();

        switch(c)
        {
            case KEYS_UP:    printf("Move FORWARDS\n");  cameraPosition = addProd(cameraPosition, scalarProd(cameraFront, MOVE));  break;
            case KEYS_DOWN:  printf("Move BACKWARDS\n"); cameraPosition = subProd(cameraPosition, scalarProd(cameraFront, MOVE));  break;
            case KEYS_RIGHT: printf("Move RIGHT\n");     cameraPosition = addProd(cameraPosition, scalarProd(normalize(crossProd(cameraFront, cameraUp)), MOVE)); break;
            case KEYS_LEFT:  printf("Move LEFT\n");      cameraPosition = subProd(cameraPosition, scalarProd(normalize(crossProd(cameraFront, cameraUp)), MOVE)); break;
            case KEYS_PGDN:  printf("Rotate UP\n");      modelRotationRight += 1.0f; break;
            case KEYS_PGUP:  printf("Rotate DOWN\n");    modelRotationRight -= 1.0f; break;
            case KEYS_HOME:  printf("Rotate LEFT\n");    modelRotationUp += 1.0f;    break;
            case KEYS_END:   printf("Rotate RIGHT\n");   modelRotationUp -= 1.0f;    break;

            case KEYS_ESC:   appShutdown = 1; break;
            default: break;
        }

        /* Update camera position */
        cameraDirection = normalize(subProd(cameraPosition, cameraTarget));
        cameraRight = normalize(crossProd(cameraUp, cameraDirection));
        cameraUp = crossProd(cameraDirection, cameraRight);

        /* Update model view projection matrix */
        updateModelViewProjMatrix();
    }
}


int main(void)
{
    GLint i;
    GLFWwindow* window;
    
    glfwInit();    
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    /* Create window */
    window = glfwCreateWindow(DISPLAY_WIDTH, DISPLAY_HEIGHT, __FILE__, NULL, NULL);

    /* Make window current */
    glfwMakeContextCurrent(window);

    /* Load Shader */
    loadShader();

    /* Create objects */
    for(i=0;i<sizeof(celestialObject)/sizeof(celestial_t); i++)
    {
        printf("Creating celestial object %d of %ld (%s)\n", i+1, sizeof(celestialObject)/sizeof(celestial_t), celestialObject[i].name);
        createCelestialObjectObject(&celestialObject[i]);
    }

    /* GL initialization */
    initGL();

    /* Loop until we need to shutdown */
    while (!glfwWindowShouldClose(window) && appShutdown == 0) 
    {
        /* Check for events */
        glfwPollEvents();

        /* Check user input */
        checkUserInput();

        /* Clear the color and depth buffer */
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

        /* Draw the objects */
        for(i=0;i<sizeof(celestialObject)/sizeof(celestial_t); i++)
        {
            drawcelestialObject(&celestialObject[i]);
        }

        /* Swap buffers */
        glfwSwapBuffers(window);
    }

    /* Clean up */
    for(i=0;i<sizeof(celestialObject)/sizeof(celestial_t); i++)
    {
        cleanUpcelestialObject(&celestialObject[i]);
    }

    glfwTerminate();

    return 0;
}

