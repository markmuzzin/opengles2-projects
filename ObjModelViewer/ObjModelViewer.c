/**********************************************************
* Solar System OpenGL ES2
* Description: 3D obj model loader
* Build command: gcc ObjModelViewer.c ../lib/glmath.c -lGLESv2 -lglfw -lm -Wall
**********************************************************/

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

#define BUFFER_OFFSET(i)            ((char *)NULL + (i))

#define ELEMENTS_PER_FACE           3
#define ELEMENTS_PER_VERTEX         3
#define ELEMENTS_PER_TEXCOORDS      2

#define VBO_VERT                    0
#define VBO_TEX                     1
#define VBO_NORM                    2

#define STRLEN                      128

#define DISPLAY_WIDTH               1024.0f
#define DISPLAY_HEIGHT              768.0f

#define BMP_WIDTH_OFFSET            18
#define BMP_HEIGHT_OFFSET           22
#define BMP_PIX_PER_COL             3
#define BMP_HEADER_SIZE             54

#define SCENE_NEAR                  0.1f
#define SCENE_FAR                   20000.0f
#define SCENE_RIGHT                 DISPLAY_WIDTH
#define SCENE_TOP                   DISPLAY_HEIGHT
#define SCENE_LEFT                  0.0f
#define SCENE_BOTTOM                0.0f

#define MOVE                        10.0f
#define MOVE_BIG                    50.0f

#define MAX_MATERIALS               32
#define MAX_MATERIAL_CHANGES        300

#define DEFAULT_CAM_DIST            100.0f
#define DEFAULT_FOV                 35.0f
#define DEFAULT_SPECULAR            0.5f
#define DEFAULT_SHININESS           32.0f

#define DEFAULT_AMBIENT             0.3f, 0.3f, 0.3f
#define DEFAULT_LIGHTSRCCOLOR       0.9f, 0.9f, 0.9f

#define KEYS_UP                     65
#define KEYS_DOWN                   'B'
#define KEYS_RIGHT                  'C'
#define KEYS_LEFT                   'D'
#define KEYS_PGDN                   '6'
#define KEYS_PGUP                   '5'
#define KEYS_HOME                   72
#define KEYS_END                    'F'
#define KEYS_ESC                    '`'

/*******************************************************************/
/*  Typedefs                                                       */
/*******************************************************************/
typedef struct _material_t
{
    char *name;
    char *fileName;
    GLuint texId;
    GLfloat Ns;
    vec3_t Ka;
    vec3_t Kd;
    vec3_t Ks;
    GLfloat Ni;
    GLfloat d;
    GLfloat illum;
} material_t;

typedef struct _material_change_t
{
    GLuint startFace;
    material_t *material;
} material_change_t;

typedef struct _object_t
{
    GLfloat *vertArray;
    GLfloat *texArray;
    GLfloat *normArray;

	GLfloat *v;
	GLuint numOfVertices;

	GLfloat *vt;
    GLuint numOfTexCoords;

	GLfloat *vn;
	GLuint numOfNormals;

    GLuint *f;
    GLuint numOfFaces;

    char *materialLibFilename;
    GLuint materialCount;

    material_change_t materialChange[MAX_MATERIAL_CHANGES];
    GLuint materialChangeCount;
} object_t;

typedef struct _objVerts3_t 
{
   GLfloat x;
   GLfloat y;
   GLfloat z;   
} objVerts3_t;

typedef struct _objTexCoords2_t 
{
   GLfloat x;
   GLfloat y;
} objTexCoords2_t;


/*******************************************************************/
/*  Enums                                                          */
/*******************************************************************/
typedef enum _retCode_e {
    RET_FAIL = -1,
    RET_SUCCESS,
} retCode_e;


/*******************************************************************/
/*  Global Variables                                               */
/*******************************************************************/
static GLuint vboids[3]                  = {0};

static GLfloat persepctiveProjMatrix[16] = {0.0f};
static GLfloat modelViewProjMatrix[16]   = {0.0f};
static GLfloat rotationMatrixUp[16]      = {0};
static GLfloat rotationMatrixRight[16]   = {0};
static GLfloat modelRotationUp           = 0.0f;
static GLfloat modelRotationRight        = 0.0f;

static vec3_t cameraRight                = {0.0f, 0.0f, 0.0f};
static vec3_t cameraDirection            = {0.0f, 0.0f, 0.0f};
static vec3_t cameraTarget               = {0.0f, 0.0f, 0.0f};

static vec3_t cameraPosition             = {0.0f, 0.0f, DEFAULT_CAM_DIST};
static vec3_t cameraFront                = {0.0f, 0.0f, -1.0f};
static vec3_t cameraUp                   = {0.0f, 1.0f,  0.0f};

static vec3_t lightPosition              = {0.0f, 200.0f, 0.0f};

static GLint shaderProgram               = -1;
static GLint aVertexLoc                  = -1;
static GLint aNormalLoc                  = -1;
static GLint aTexCoordsLoc               = -1;

static GLint uTextureColorLoc            = -1;
static GLint uMVPLoc                     = -1;
static GLint uViewPositionLoc            = -1;
static GLint uSpecularStrengthLoc        = -1;

static GLint uShininessLoc               = -1;
static GLint uLightPosLoc                = -1;
static GLint uAmbientLightLoc            = -1;
static GLint uLightSrcColorLoc           = -1;
static GLint uKaLoc                      = -1;
static GLint uKdLoc                      = -1;
static GLint uKsLoc                      = -1;
static GLint uDLoc                       = -1;

static GLint appShutdown                 = 0;

static const GLchar* vertex_shader_source =    
{
    "precision mediump float;\n"

    "attribute vec2 aTexCoord;\n"
    "attribute vec3 aPosition;\n"
    "attribute vec3 aNormal;\n"

    "varying vec2 vTexCoord;\n"
    "varying vec3 vPosition;\n"
    "varying vec3 vNormal;\n"

    "uniform mat4 uMVP;\n"

    "void main(void)\n"
    "{\n"
        "vTexCoord = aTexCoord;\n"
        "vPosition = aPosition;\n"
        "vNormal = aNormal;\n"
        "gl_Position = vec4(vPosition, 1.0) * uMVP;\n"
    "}\n"
};

static const GLchar* fragment_shader_source =
{
    "precision mediump float;\n"

    "uniform vec3 uLightPos;\n"
    "uniform vec3 uViewPosition;\n"
    "uniform sampler2D uTextureColor;\n"

    "uniform float uShininess;\n"
    "uniform vec3 uAmbientLight;\n"

    "uniform vec3 uLightSrcColor;\n"

    "uniform vec3 uKa;\n"
    "uniform vec3 uKs;\n"
    "uniform vec3 uKd;\n"
    "uniform float uD;\n"

    "varying vec3 vPosition;\n;"
    "varying vec2 vTexCoord;\n;"
    "varying vec3 vNormal;\n;"

    "void main(void)\n"
    "{\n"
        "vec3 Ka = clamp(uKa, -1.0, 1.0)\n;"
        "vec3 Ks = clamp(uKs, -1.0, 1.0)\n;"
        "vec3 Kd = clamp(uKd, -1.0, 1.0)\n;"
        "float d = clamp(uD, -1.0, 1.0)\n;"

        "vec3 normal = normalize(vNormal);\n"

        "vec3 lightDirection = normalize(uLightPos - vPosition);\n"
        "vec3 viewDirection = normalize(uViewPosition - vPosition);\n"
        "vec3 reflectDirection = reflect(-lightDirection, normal);\n"

        "vec3 diffuse = (Ka + uLightSrcColor) * Kd * max(dot(normal, lightDirection), 0.0);\n"
        "vec3 specular = Ks * pow(max(dot(viewDirection, reflectDirection), 0.0), uShininess);\n"

        "vec4 color = texture2D(uTextureColor, vTexCoord);\n"

        "gl_FragColor = color.bgra * vec4(diffuse + specular + uAmbientLight, d);\n"

   "}\n"
};

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
    {
        perror("tcsetattr()");
    }    

    old.c_lflag &= ~ICANON;
    old.c_lflag &= ~ECHO;
    old.c_cc[VMIN] = 1;
    old.c_cc[VTIME] = 0;

    if(tcsetattr(0, TCSANOW, &old) < 0) 
    {
        perror("tcsetattr ICANON");
    }

    if(read(0, &buf, 1) < 0)
    {
        perror("read()");
    }    
    old.c_lflag |= ICANON;
    old.c_lflag |= ECHO;

    if(tcsetattr(0, TCSADRAIN, &old) < 0)
    {
        perror("tcsetattr ~ICANON");
    }    
    return buf;
}


GLint loadShaderProgram(const char *vertex_shader_source, const char *fragment_shader_source) 
{
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
    if (!success) 
    {
        glGetShaderInfoLog(vertex_shader, INFOLOG_LEN, NULL, infoLog);
        printf("ERROR::SHADER::VERTEX::COMPILATION_FAILED\n%s\n", infoLog);
    }

    /* Fragment shader */
    fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragment_shader_source, NULL);
    glCompileShader(fragment_shader);
    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
    if (!success) 
    {
        glGetShaderInfoLog(fragment_shader, INFOLOG_LEN, NULL, infoLog);
        printf("ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n%s\n", infoLog);
    }

    /* Link shaders */
    shader_program = glCreateProgram();
    glAttachShader(shader_program, vertex_shader);
    glAttachShader(shader_program, fragment_shader);
    glLinkProgram(shader_program);
    glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
    if (!success) 
    {
        glGetProgramInfoLog(shader_program, INFOLOG_LEN, NULL, infoLog);
        printf("ERROR::SHADER::PROGRAM::LINKING_FAILED\n%s\n", infoLog);
    }

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    return shader_program;
}


void loadShader(void)
{
    /* Load shader program */
    shaderProgram = loadShaderProgram(vertex_shader_source, fragment_shader_source);

    /* Get the vertex attribute and color uniform locations */
    aVertexLoc = glGetAttribLocation(shaderProgram, "aPosition");
    aTexCoordsLoc = glGetAttribLocation(shaderProgram, "aTexCoord");
    aNormalLoc = glGetAttribLocation(shaderProgram, "aNormal");

    uTextureColorLoc = glGetUniformLocation(shaderProgram, "uTextureColor");
    uMVPLoc = glGetUniformLocation(shaderProgram, "uMVP");
    uLightPosLoc = glGetUniformLocation(shaderProgram, "uLightPos");
    uAmbientLightLoc = glGetUniformLocation(shaderProgram, "uAmbientLight");
    uViewPositionLoc = glGetUniformLocation(shaderProgram, "uViewPosition");
    uSpecularStrengthLoc = glGetUniformLocation(shaderProgram, "uSpecularStrength");

    uKaLoc = glGetUniformLocation(shaderProgram, "uKa");
    uKdLoc = glGetUniformLocation(shaderProgram, "uKd");
    uKsLoc = glGetUniformLocation(shaderProgram, "uKs");
    uDLoc = glGetUniformLocation(shaderProgram, "uD");

    uLightSrcColorLoc = glGetUniformLocation(shaderProgram, "uLightSrcColor");


    /* Use program */
    glUseProgram(shaderProgram);

    /* Bind uniform samplers to texture units */
    glUniform1i(uTextureColorLoc, 0);
}


GLboolean loadTexture(material_t *materials, char *path)
{
    GLubyte *data = NULL;
    FILE *pFile = NULL;
    char *fullPath;
    
    fullPath = malloc(strlen(path) + strlen(materials->fileName));
    memset(fullPath, 0, strlen(path) + strlen(materials->fileName));
    strcpy(fullPath, path);
    strcat(fullPath, materials->fileName);

    pFile = fopen(fullPath, "rb");
    if (pFile == NULL)
    {
       printf("Error opening TEX file\n");
       return GL_FALSE;
    }

    /* Read file */
    data = (GLubyte *)malloc( BMP_HEADER_SIZE );

    /* Read BMP Header */
    fread(data, BMP_HEADER_SIZE, 1, pFile);

    /* Get width and height from header */
    GLuint width = *(GLuint *)(&data[BMP_WIDTH_OFFSET]);
    GLuint height = *(GLuint *)(&data[BMP_HEIGHT_OFFSET]);

    /* Read pixel data */
    data = (GLubyte *)realloc( data, width * height * 3 );
    fread( data, width * height * BMP_PIX_PER_COL, 1, pFile );
    fclose( pFile );

    /* Generate and store texture */
    glGenTextures(1, &materials->texId);
    glBindTexture(GL_TEXTURE_2D, materials->texId);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB8, width, height);
    glTexSubImage2D(GL_TEXTURE_2D, 0,0,0, width, height, GL_RGB, GL_UNSIGNED_BYTE, data);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );

    /* Free data */
    free(data);
    free(fullPath);

    return GL_TRUE;
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

    /* Enable depth test */
    glEnable(GL_DEPTH_TEST);
    
    /* Set depth function to less than equal */
    glDepthFunc(GL_LESS);
    
    /* Set depth range */
    glDepthRangef(0.0f, 1.0f);

    /* Set clear color */
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

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

    /* Generate VBO buffers */
    glGenBuffers(3, vboids);

    /* Load spotligt position */
    glUniform3fv(uLightPosLoc, 1, (GLfloat*)&lightPosition);

    /* Load ambient light value */
    glUniform3f(uAmbientLightLoc, DEFAULT_AMBIENT);

    /* Load specular strength */
    glUniform1f(uSpecularStrengthLoc, DEFAULT_SPECULAR);

    /* Load shineness */
    glUniform1f(uShininessLoc, DEFAULT_SHININESS);

    /* Load light source diffuse */
    glUniform3f(uLightSrcColorLoc, DEFAULT_LIGHTSRCCOLOR);
}


void updateCameraPosition(void)
{
    /* Update camera position */
    cameraDirection = normalize(subProd(cameraPosition, cameraTarget));
    cameraRight = normalize(crossProd(cameraUp, cameraDirection));
    cameraUp = crossProd(cameraDirection, cameraRight);

    /* Update model view projection matrix */
    updateModelViewProjMatrix();

    /* Load camera target */
    glUniformMatrix3fv(uViewPositionLoc, 1, 0, (GLfloat *)&cameraTarget);
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
        updateCameraPosition();
    }
}



void addFloatData(GLfloat **buffer, GLfloat *data, GLuint *numOfData, GLuint count)
{
    GLuint i;
    for(i=0;i<count;i++)
    {
        *buffer = (GLfloat *)realloc(*buffer, ((*numOfData)+1) * sizeof(GLfloat));
        *(*buffer+(*numOfData)) = *(data+i);
        (*numOfData)++;
    }
}


void addIntegerData(GLuint **buffer, GLint *data, GLuint *numOfData, GLuint count)
{
    GLuint i;
    
    for(i=0;i<count;i++)
    {
        *buffer = (GLuint *)realloc(*buffer, ((*numOfData)+1) * sizeof(GLuint));
        *(*buffer+(*numOfData)) = *(data+i);
        (*numOfData)++;
    }
}


GLboolean checkPrefix(char *line, char *expPrefix)
{
    return (strncmp(line, expPrefix, strlen(expPrefix)) == 0) ? GL_TRUE : GL_FALSE;
}


void setMaterialDefaults(material_t *material)
{
    material->Ns = 100.0f;

    material->Ka.x = 0.0f;
    material->Ka.x = 0.0f;
    material->Ka.x = 0.0f;

    material->Kd.x = 1.0;
    material->Kd.y = 1.0;
    material->Kd.z = 1.0;

    material->Ks.x = 0.01f;
    material->Ks.y = 0.01f;
    material->Ks.z = 0.01f;

    material->Ni = 1.0f;
    material->d = 1.0f;
    material->illum = 1.0f;
}


GLboolean loadMtlFile(object_t *object, material_t *materials, char *mtlFilename)
{
 	char line[STRLEN];
    char stringName[STRLEN];
    char keyword[STRLEN];
    FILE *pFile;
    GLuint i;
    GLboolean endOfEntry = GL_FALSE;

    pFile = fopen(mtlFilename, "rb");
    if (pFile == NULL)
    {
       printf("Error opening MTL file\n");
       return GL_FALSE;
    }

    /* Get the line entry */
    while(fgets(line, STRLEN, pFile) != NULL)
    {
        if ( checkPrefix(line, "newmtl ") )
        {
            endOfEntry = GL_FALSE;
            sscanf(line, "%s %s", keyword, stringName);

            for(i=0;i<object->materialCount && !endOfEntry;i++)
            {
                /* Check if newmtl matches one of the names in the material list */                        
                if( 0 == strcmp(materials[i].name, stringName) )
                {
                    /* Set the default values in case the material file does not specify them */
                    setMaterialDefaults(&materials[i]);

                    /* If so, cycle through each entry until a blank line is found */
                    while(!endOfEntry)
                    {
                        memset(line, 0, STRLEN);
                        fgets(line, STRLEN, pFile);

                        if( checkPrefix(line, "Ns ") )
                        {
                            sscanf(line, "%s %f", keyword, &materials[i].Ns);
                        }
                        else if( checkPrefix(line, "Ka ") )
                        {
                            sscanf(line, "%s %f %f %f", keyword, &materials[i].Ka.x, &materials[i].Ka.y, &materials[i].Ka.z);
                        }
                        else if( checkPrefix(line, "Kd ") )
                        {
                            sscanf(line, "%s %f %f %f", keyword, &materials[i].Kd.x, &materials[i].Kd.y, &materials[i].Kd.z);
                        }
                        else if( checkPrefix(line, "Ks ") )
                        {
                            sscanf(line, "%s %f %f %f", keyword, &materials[i].Ks.x, &materials[i].Ks.y, &materials[i].Ks.z);
                        }
                        else if( checkPrefix(line, "Ni ") )
                        {
                            sscanf(line, "%s %f", keyword, &materials[i].Ni);
                        }
                        else if( checkPrefix(line, "d ") )
                        {
                            sscanf(line, "%s %f", keyword, &materials[i].d);
                        }
                        else if( checkPrefix(line, "illum ") )
                        {
                            sscanf(line, "%s %f", keyword, &materials[i].illum);
                        }
                        else if( checkPrefix(line, "map_Kd ") )
                        {
                            sscanf(line, "%s %s", keyword, stringName);
                            materials[i].fileName = (char *)malloc(strlen(stringName));
                            strcpy(materials[i].fileName, stringName);                                    
                        }
                        else
                        {
                            /* Check for blank line */
                            if ( strlen(line) <= 2 )
                            {
                                  endOfEntry = GL_TRUE;
                            }
                        }
                    }
                }
            }
        }
    }

    fclose(pFile);

    return GL_TRUE;
}


GLboolean loadObjFile(object_t *object, material_t *materials, char *objFilename)
{
    char prefix[STRLEN];
 	char line[STRLEN];
    char stringName[STRLEN];
    char keyword[STRLEN];
    GLfloat value[3];
    GLint indices[9];
    FILE *pFile = NULL;
    GLboolean haveTexture = GL_FALSE;
    GLuint i;

    /* Open object file */
    pFile = fopen(objFilename, "r");

    if (pFile == NULL)
    {
       printf("Error opening OBJ file\n");
       return GL_FALSE;
    }

    /* Get the line entry */
    while(fgets(line, STRLEN, pFile) != NULL)
    {       
        if( checkPrefix(line, "v ") )
        {
            sscanf(line, "%s %f %f %f", prefix, &value[0], &value[1], &value[2]);
            addFloatData(&object->v, value, &object->numOfVertices, 3); 
        }
        else if( checkPrefix(line, "vt ") )
        {
            sscanf(line, "%s %f %f", prefix, &value[0], &value[1]);
            addFloatData(&object->vt, value, &object->numOfTexCoords, 2); 
        }
        else if( checkPrefix(line, "vn ") )
        {
            sscanf(line, "%s %f %f %f", prefix, &value[0], &value[1], &value[2]);
            addFloatData(&object->vn, value, &object->numOfNormals, 3); 
        }
        else if( checkPrefix(line, "f ") )
        {
            if(object->numOfNormals == 0)
            {
                sscanf(line, "%s %d/%d %d/%d %d/%d", prefix, 
                                &indices[0], &indices[1],
                                &indices[2], &indices[3],
                                &indices[4], &indices[5]);

                addIntegerData(&object->f, indices, &object->numOfFaces, 6);
            }
            else
            {
                sscanf(line, "%s %d/%d/%d %d/%d/%d %d/%d/%d", prefix, 
                                &indices[0], &indices[1], &indices[2],
                                &indices[3], &indices[4], &indices[5],
                                &indices[6], &indices[7], &indices[8]);
                addIntegerData(&object->f, indices, &object->numOfFaces, 9);
            }
        }
        else if( checkPrefix(line, "usemtl ") )
        {
            /* Reset flag */
            haveTexture = GL_FALSE;

            sscanf(line, "%s %s", keyword, stringName);

            /* Check if we have this texture */
            for(i=0;i<object->materialCount;i++)
            {
                if(0 == strcmp(materials[i].name, stringName)) 
                {
                    /* We have this material already */
                    haveTexture = GL_TRUE;

                    /* Mark the start face that uses this texture */
                    object->materialChange[object->materialChangeCount].startFace = object->numOfFaces;
                    object->materialChange[object->materialChangeCount].startFace /= (ELEMENTS_PER_FACE*ELEMENTS_PER_VERTEX);
                    object->materialChange[object->materialChangeCount].material = &materials[i];
                    object->materialChangeCount++;
                    break;
                }
            }

            /* If we dont have this texture name stored yet */
            if ( GL_FALSE == haveTexture ) 
            {
                /* Add material name */
                materials[object->materialCount].name = (char *)malloc(strlen(stringName));
                memset(materials[object->materialCount].name, 0, strlen(stringName));
                strcpy(materials[object->materialCount].name, stringName);

                /* Mark the start face that uses this texture */
                object->materialChange[object->materialChangeCount].startFace = object->numOfFaces;
                object->materialChange[object->materialChangeCount].startFace /= (ELEMENTS_PER_FACE*ELEMENTS_PER_VERTEX);
                object->materialChange[object->materialChangeCount].material = &materials[object->materialCount];
                object->materialChangeCount++;

                /* Increment material count */
                object->materialCount++;
            }
        }
        else if( checkPrefix(line, "mtllib ") )
        {
            sscanf(line, "%s %s", keyword, stringName);

            /* Add material filename */
            object->materialLibFilename = (char *)malloc(strlen(stringName)+1);
            memset(object->materialLibFilename, 0, strlen(stringName)+1);
            strcpy(object->materialLibFilename, stringName);
        }
        else if( checkPrefix(line, "lightpos ") )
        {
            sscanf(line, "%s %f %f %f", prefix, &lightPosition.x, &lightPosition.y, &lightPosition.z);
            /* Load spotligt position */
            glUniform3fv(uLightPosLoc, 1, (GLfloat*)&lightPosition);
        }
        else if( checkPrefix(line, "campos ") )
        {
            sscanf(line, "%s %f %f %f", prefix, &cameraPosition.x, &cameraPosition.y, &cameraPosition.z);
        }
        else if( checkPrefix(line, "camfront ") )
        {
            sscanf(line, "%s %f %f %f", prefix, &cameraFront.x, &cameraFront.y, &cameraFront.z);
        }
        else if( checkPrefix(line, "camup ") )
        {
            sscanf(line, "%s %f %f %f", prefix, &cameraUp.x, &cameraUp.y, &cameraUp.z);
        }
        else
        {
        
        }
    }

    object->numOfVertices /= ELEMENTS_PER_VERTEX;
    object->numOfTexCoords /= ELEMENTS_PER_TEXCOORDS;
    object->numOfNormals /= ELEMENTS_PER_VERTEX;

    /* Check if we have normals in the file, calculate the face count accordingly */
    object->numOfFaces /= (object->numOfNormals) ? (ELEMENTS_PER_FACE*3) : (ELEMENTS_PER_FACE*2);
  
    fclose (pFile);

    return GL_TRUE;
}


void drawVertices(object_t *object, material_t *materials)
{
    GLint i;
    GLuint faceCount;
    GLuint endFace;
    
    for(i=0;i<object->materialChangeCount;i++)
    {
        /* Calculate the end face for this material */
        endFace = (i < object->materialChangeCount - 1) ? object->materialChange[i+1].startFace : object->numOfFaces;

        /* If a texture is specified, bind it */
        if(object->materialChange[i].material->texId != -1)
        {
            glBindTexture(GL_TEXTURE_2D, object->materialChange[i].material->texId);
        }

        /* Apply material parameters */
        glUniform3fv(uKaLoc, 1, (GLfloat *)&object->materialChange[i].material->Ka);
        glUniform3fv(uKdLoc, 1, (GLfloat *)&object->materialChange[i].material->Kd);
        glUniform3fv(uKsLoc, 1, (GLfloat *)&object->materialChange[i].material->Ks);
        glUniform1f(uDLoc, object->materialChange[i].material->d);

        /* Check dissolve factor */
        if (object->materialChange[i].material->d != 1.0f) 
        {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }
        else
        {
            glDisable(GL_BLEND);
        }

        /* Calculate the face count for this material */
        faceCount = (endFace - object->materialChange[i].startFace);

        /* Draw faces */
        glDrawArrays(GL_TRIANGLES, object->materialChange[i].startFace*ELEMENTS_PER_FACE, faceCount*ELEMENTS_PER_FACE);
    }
}


void prepareObjectArrays(object_t *object)
{
    GLint i, j;
    GLint v[3];
    GLint vt[3];
    GLint vn[3];
    GLuint vc = 0, tc = 0, vnc = 0;

    /* Assign pointers to input values */
    GLuint *faces = (GLuint *)object->f;

    /* Create vertex and texture buffers */
    object->vertArray = (GLfloat *)malloc(sizeof(GLfloat)*object->numOfFaces*(ELEMENTS_PER_FACE*ELEMENTS_PER_VERTEX));
    object->texArray = (GLfloat *)malloc(sizeof(GLfloat)*object->numOfFaces*(ELEMENTS_PER_FACE*ELEMENTS_PER_TEXCOORDS));
    object->normArray = (GLfloat *)malloc(sizeof(GLfloat)*object->numOfFaces*(ELEMENTS_PER_FACE*ELEMENTS_PER_VERTEX));
    
    GLuint stride = (object->numOfNormals == 0) ? (ELEMENTS_PER_FACE*ELEMENTS_PER_TEXCOORDS) : (ELEMENTS_PER_FACE*ELEMENTS_PER_VERTEX);
    GLuint offset = (object->numOfNormals == 0) ? 0 : 1;

    for(i=0;i<object->numOfFaces;i++)
    {
        /* Get offsets from face inputs */
        v[0]  = faces[(stride*i)+0]-1;
        vt[0] = faces[(stride*i)+1]-1;
        v[1]  = faces[(stride*i)+2+offset]-1;
        vt[1] = faces[(stride*i)+3+offset]-1;
        v[2]  = faces[(stride*i)+4+(offset*2)]-1;           
        vt[2] = faces[(stride*i)+5+(offset*2)]-1;
        
        /* If this file contains normals */
        if(object->numOfNormals != 0)
        {
            vn[0] = faces[(stride*i)+2]-1;
            vn[1] = faces[(stride*i)+5]-1;
            vn[2] = faces[(stride*i)+8]-1;

            for(j=0; j<3; j++)
            {
                object->normArray[vnc++] = *(object->vn + (vn[j]*ELEMENTS_PER_VERTEX + 0)); 
                object->normArray[vnc++] = *(object->vn + (vn[j]*ELEMENTS_PER_VERTEX + 1)); 
                object->normArray[vnc++] = *(object->vn + (vn[j]*ELEMENTS_PER_VERTEX + 2)); 
            }
        }

        /* Load vertices */
        for(j=0; j<3; j++)
        {
            object->vertArray[vc++] = *(object->v + (v[j]*ELEMENTS_PER_VERTEX + 0));  
            object->vertArray[vc++] = *(object->v + (v[j]*ELEMENTS_PER_VERTEX + 1));
            object->vertArray[vc++] = *(object->v + (v[j]*ELEMENTS_PER_VERTEX + 2));
        }

        /* Load tex coords */
        for(j=0; j<3; j++)
        {
            object->texArray[tc++] = *(object->vt + (vt[j]*ELEMENTS_PER_TEXCOORDS + 0));  
            object->texArray[tc++] = *(object->vt + (vt[j]*ELEMENTS_PER_TEXCOORDS + 1));  
        }
    }
}


void cleanUp(object_t *object, material_t *material)
{
    GLuint i;

    glDisableVertexAttribArray(aVertexLoc);
    glDisableVertexAttribArray(aNormalLoc);
    glDisableVertexAttribArray(aTexCoordsLoc);

    for(i=0;i<object->materialCount;i++)
    {
        if ( (material+i)->fileName != NULL )
        {
            free((material+i)->fileName);
        }
    }


    if( object->materialLibFilename != NULL )
    {
        free(object->materialLibFilename);
    }

    if( object->vertArray != NULL )
    {
        free(object->vertArray);
    }

    if( object->texArray != NULL )
    {
        free(object->texArray);
    }

    if( object->normArray != NULL )
    {
        free(object->normArray);
    }
}


char* getPath(char *string)
{
    char *path = NULL;
    GLint position = strlen(string);    

    while(position != 0 && string[--position] != '/');

    path = (char *)malloc(position+2);
    memset(path, 0, position+2);
    strncpy(path, string, position+1);

    return path;
}


GLboolean loadModel(object_t *object, material_t *materials, char *objFileName)
{
    GLint i;
    char *mtlFile;
    char *path;

    mtlFile = getPath(objFileName);
    path = getPath(objFileName);

    /* Load and parse obj file */
    printf("Loading object file: %s...", objFileName);

    if ( GL_FALSE == loadObjFile(object, materials, objFileName) )
    {
        cleanUp(object, materials);
        return GL_FALSE;
    }
    printf("done\n");

    /* Construct material filenamd */
    mtlFile = (char*)realloc(mtlFile, (strlen(mtlFile) + strlen(object->materialLibFilename))+1);
    strncat(mtlFile, object->materialLibFilename, strlen(object->materialLibFilename)+1);

    /* Load and parse material file */
    printf("Loading mtl file: %s...", mtlFile);
    if ( GL_FALSE == loadMtlFile(object, materials, mtlFile) )
    {
        return GL_FALSE;
    }
    printf("done\n");

    printf("\tvertices:   %d\n", object->numOfVertices);
    printf("\ttex coords: %d\n", object->numOfTexCoords);
    printf("\tnormals:    %d\n", object->numOfNormals);
    printf("\tfaces:      %d\n", object->numOfFaces);
    printf("\tmaterials:  %d\n", object->materialCount);

    for(i=0; i<object->materialCount; i++)
    {
        printf("\t%3d:   %s\tfilename: %s\n", i, materials[i].name, materials[i].fileName);
    }

    /* Load texture files */
    for(i=0;i<object->materialCount;i++)
    {
        if(materials[i].fileName != NULL)
        {
            printf("Loading tex files: %s...", materials[i].fileName);
            if ( GL_FALSE == loadTexture(&materials[i], path) )
            {
                return GL_FALSE;
            }
            printf("done\n");
        }
    }

    free(mtlFile);
    free(path);

    return GL_TRUE;
}


void prepareVbos(object_t *object)
{
    glBindBuffer(GL_ARRAY_BUFFER, vboids[VBO_VERT]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*object->numOfFaces*(ELEMENTS_PER_FACE*ELEMENTS_PER_VERTEX), object->vertArray, GL_STATIC_DRAW);
    glVertexAttribPointer(aVertexLoc, ELEMENTS_PER_VERTEX, GL_FLOAT, 0, 0, BUFFER_OFFSET(0));

    glBindBuffer(GL_ARRAY_BUFFER, vboids[VBO_TEX]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*object->numOfFaces*(ELEMENTS_PER_FACE*ELEMENTS_PER_TEXCOORDS), object->texArray, GL_STATIC_DRAW);
    glVertexAttribPointer(aTexCoordsLoc, ELEMENTS_PER_TEXCOORDS, GL_FLOAT, 0, 0, BUFFER_OFFSET(0));

    glBindBuffer(GL_ARRAY_BUFFER, vboids[VBO_NORM]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*object->numOfFaces*(ELEMENTS_PER_FACE*ELEMENTS_PER_VERTEX), object->normArray, GL_STATIC_DRAW);
    glVertexAttribPointer(aNormalLoc, ELEMENTS_PER_VERTEX, GL_FLOAT, 0, 0, BUFFER_OFFSET(0));

    glEnableVertexAttribArray(aVertexLoc);
    glEnableVertexAttribArray(aNormalLoc);
    glEnableVertexAttribArray(aTexCoordsLoc);
}


int main(int argc, char **argv)
{
    object_t object = { 0 };
    material_t materials[MAX_MATERIALS] = { 0 };

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

    /* Set camera distance if provided */
    if (argc > 2)
    {
        cameraPosition.z = atof(argv[2]);
    }

    /* Load model */
    if ( GL_FALSE == loadModel(&object, materials, argv[1]) )
    {
        printf("\nError loading model\n");
        return -1;
    }

    /* GL initialization */
    initGL();

    /* Prepare vertex and texture arrays */
    prepareObjectArrays(&object);

    /* Prepare VBOs */
    prepareVbos(&object);

    /* Loop until we need to shutdown */
    while (!glfwWindowShouldClose(window) && appShutdown == 0) 
    {
        /* Check for events */
        glfwPollEvents();

        /* Check user input */
        checkUserInput();

        /* Clear the color and depth buffer */
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

        /* Draw the object */
        drawVertices(&object, materials);

        /* Rotate the object */
        modelRotationUp -= 0.5f;
        if(modelRotationUp > 360.0f)
        {
            modelRotationUp = modelRotationUp-360.0f;
        }

        /* Update camera position */
        updateCameraPosition();

        /* Swap buffers */
        glfwSwapBuffers(window);
    }

    glfwTerminate();

    cleanUp(&object, materials);

    return 0;
}

