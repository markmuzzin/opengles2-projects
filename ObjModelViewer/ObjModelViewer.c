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

#define V                           (1<<0)
#define VT                          (1<<1)
#define VN                          (1<<2)
#define F                           (1<<3)
#define USEMTL                      (1<<4)
#define NEWMTL                      (1<<5)
#define MAP_KD                      (1<<6)


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

#define DEFAULT_CAM_DIST            100.0f

#define SCENE_NEAR                  0.1f
#define SCENE_FAR                   20000.0f
#define SCENE_RIGHT                 DISPLAY_WIDTH
#define SCENE_TOP                   DISPLAY_HEIGHT
#define SCENE_LEFT                  0.0f
#define SCENE_BOTTOM                0.0f

#define MOVE                        20.0f
#define MOVE_BIG                    100.0f

#define DEFAULT_FOV                 35.0f

#define MAX_MATERIALS               32

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
    GLuint numOfElementsPerVertices;

	GLfloat *vt;
    GLuint numOfTexCoords;
    GLuint numOfElementsPerTexCoords;

	GLfloat *vn;
	GLuint numOfNormals;
    GLuint numOfElementsPerNormals;

    GLuint *f;
    GLuint numOfFaces;
    GLuint numOfIndicesPerFace;

    GLuint materialCount;
    material_change_t materialChange[300];
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
static GLfloat modelRotationUp           = 14.0f;
static GLfloat modelRotationRight        = 9.0f;

static vec3_t cameraRight                = {0.0f, 0.0f, 0.0f};
static vec3_t cameraDirection            = {0.0f, 0.0f, 0.0f};
static vec3_t cameraTarget               = {0.0f, 0.0f, 0.0f};

static vec3_t cameraPosition             = {0.0f, 0.0f, DEFAULT_CAM_DIST};
static vec3_t cameraFront                = {0.0f, 0.0f, -1.0f};
static vec3_t cameraUp                   = {0.0f, 1.0f,  0.0f};

static vec3_t lightPosition              = {10.0f, 15.0f, -30.0f};

static GLint shaderProgram               = -1;
static GLint aVertexLoc                  = -1;
static GLint aNormalLoc                  = -1;
static GLint aTexCoordsLoc               = -1;

static GLint uTextureColorLoc            = -1;
static GLint uMVPLoc                     = -1;
static GLint uLightPosLoc                = -1;

static GLint appShutdown                 = 0;

static const GLchar* vertex_shader_source =    
{
    "precision mediump float;\n"

    "attribute vec2 aTexCoords;\n"
    "attribute vec3 aPosition;\n"
    "attribute vec3 aNormals;\n"

    "varying vec2 vTexCoords;\n"
    "varying vec3 vPosition;\n"
    "varying vec3 vNormals;\n"

    "uniform mat4 uMVP;\n"

    "void main(void)\n"
    "{\n"
        "vTexCoords = aTexCoords;\n"
        "vPosition = aPosition;\n"
        "vNormals = aNormals;\n"
        "gl_Position = vec4(vPosition, 1.0) * uMVP;\n"
    "}\n"
};


static const GLchar* fragment_shader_source =
{
    "precision mediump float;\n"

    "uniform vec3 uLightPos;\n"
    "uniform sampler2D uTextureColor;\n"

    "varying vec3 vPosition;\n"
    "varying vec2 vTexCoords;\n"
    "varying vec3 vNormals;\n"

    "void main(void)\n"
    "{\n"
        "vec4 color = texture2D(uTextureColor, vTexCoords);\n"
        "vec3 lightDirection = normalize(uLightPos - vPosition);\n"
        "float diffuse = max(dot(vPosition, lightDirection), 0.0);\n"
        "float distance = length(uLightPos - vPosition);\n"

        "diffuse = diffuse * (500.0 / (1.0 + (1.25 * (distance * distance))));\n"

        "gl_FragColor = color.bgra * (0.8+diffuse);\n"
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


void loadShader(void)
{
    /* Load shader program */
    shaderProgram = loadShaderProgram(vertex_shader_source, fragment_shader_source);

    /* Get the vertex attribute and color uniform locations */
    aVertexLoc = glGetAttribLocation(shaderProgram, "aPosition");
    aTexCoordsLoc = glGetAttribLocation(shaderProgram, "aTexCoords");
    aNormalLoc = glGetAttribLocation(shaderProgram, "aNormals");
    uTextureColorLoc = glGetUniformLocation(shaderProgram, "uTextureColor");
    uMVPLoc = glGetUniformLocation(shaderProgram, "uMVP");
    uLightPosLoc = glGetUniformLocation(shaderProgram, "uLightPos");

    /* Use program */
    glUseProgram(shaderProgram);

    /* Bind uniform samplers to texture units */
    glUniform1i(uTextureColorLoc, 0);
}


GLboolean loadTexture(material_t *materials)
{
    GLubyte *data = NULL;
    FILE *pFile = NULL;

    pFile = fopen(materials->fileName, "rb");
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
    glClearColor(0.5f, 0.5f, 0.5f, 1.0f);

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
}


void updateCameraPosition(void)
{
    /* Update camera position */
    cameraDirection = normalize(subProd(cameraPosition, cameraTarget));
    cameraRight = normalize(crossProd(cameraUp, cameraDirection));
    cameraUp = crossProd(cameraDirection, cameraRight);

    /* Update model view projection matrix */
    updateModelViewProjMatrix();
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


void addFloatData(GLfloat **buffer, char *data, GLuint *numOfData)
{
    *buffer = (GLfloat *)realloc(*buffer, ((*numOfData)+1) * sizeof(GLfloat));
    *(*buffer+(*numOfData)) = atof(data);
    (*numOfData)++;
}


void addIntegerData(GLuint **buffer, char *data, GLuint *numOfData)
{
    *buffer = (GLuint *)realloc(*buffer, ((*numOfData)+1) * sizeof(GLuint));
    *(*buffer+(*numOfData)) = atoi(data);
    (*numOfData)++;
}


GLboolean loadMtlFile(object_t *object, material_t *materials, char *mtlFilename)
{
    GLuint i;
    FILE *pFile;
    char *token;
    char *lineString;
    char line[STRLEN];

    pFile = fopen(mtlFilename, "rb");
    if (pFile == NULL)
    {
       printf("Error opening MTL file\n");
       return GL_FALSE;
    }

    /* Get the line entry */
    while(fgets(line, STRLEN, pFile) != NULL)
    {
        GLuint newmtl = (strncmp(line, "newmtl ", 7) == 0) ? NEWMTL : 0;
        GLuint mapKd  = (strncmp(line, "map_Kd ", 7) == 0) ? MAP_KD : 0;

        GLuint entry = newmtl | mapKd;

        /* Parse the line */
        token = strtok_r(line, " ", &lineString);

        while ((token = strtok_r(NULL, " ", &lineString)) != NULL)
        {
            switch (entry)
            {
                case NEWMTL:
                    for(i=0;i<object->materialCount;i++)
                    {
                        /* Check if newmtl matches one of the names in the material list */                        
                        if( 0 == strncmp(materials[i].name, token, strlen(materials[i].name)))
                        {
                            /* TODO This needs to be reworked to get all of the material properties */
                            /* If so, cycle through each entry until a blank line is found */
                            while(fgets(line, STRLEN, pFile) != NULL )
                            {
                                if ( 0 == strncmp(line, "map_Kd ", 7))
                                { 
                                    GLubyte cr = (strstr(line, "\r") == 0) ? 0 : 1;
                                    GLubyte lf = (strstr(line, "\n") == 0) ? 0 : 1;
                                    materials[i].fileName = (char *)malloc(sizeof(GLbyte) * (strlen(line)-7) );
                                    memset(materials[i].fileName, 0, sizeof(GLbyte)*(strlen(line)-7));
                                    strncpy(materials[i].fileName, line+7, (strlen(line)-7-cr-lf));
                                    break;
                                }
                            }
                        }
                    }
                    break;

                case MAP_KD:
                    /* Get the material file associated with name */ 
                    break;

                default:
                    break;
            }

        }
    }

    fclose(pFile);

    return GL_TRUE;
}



GLboolean loadObjFile(object_t *object, material_t *materials, char *objFilename)
{
    GLboolean haveTexture = GL_FALSE;
    GLuint numOfElements = 0;
    FILE *pFile = NULL;
    GLuint i;
 	char line[STRLEN];
    char *token;
    char *subToken;
    char *lineString;
    char *entryString;

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
        GLuint v       = (strncmp(line, "v ",       2) == 0) ? V       : 0;
        GLuint vt      = (strncmp(line, "vt ",      3) == 0) ? VT      : 0;
        GLuint vn      = (strncmp(line, "vn ",      3) == 0) ? VN      : 0;
        GLuint f       = (strncmp(line, "f ",       2) == 0) ? F       : 0;
        GLuint usemtl  = (strncmp(line, "usemtl ",  7) == 0) ? USEMTL  : 0;

        numOfElements = 0;

        GLuint entry = v | vt | vn | f | usemtl;

        token = strtok_r(line, " ", &lineString);
                    
        while ((token = strtok_r(NULL, " ", &lineString)) != NULL )
        {   
            switch (entry)
            {
                case V:
                    addFloatData(&object->v, token, &object->numOfVertices); 
                    numOfElements++;
                    break;

                case VT:
                    addFloatData(&object->vt, token, &object->numOfTexCoords); 
                    numOfElements++;
                    break;

                case VN:
                    addFloatData(&object->vn, token, &object->numOfNormals); 
                    numOfElements++; 
                    break;

                case F:
                    while(token != NULL) 
                    {
                        subToken = strtok_r(token, "/", &entryString);

                        while (subToken != NULL) 
                        {
                            addIntegerData(&object->f, subToken, &object->numOfFaces); 
                            numOfElements++;
                            subToken = strtok_r(NULL, "/", &entryString);
                        }

                        token = strtok_r(NULL, " ", &entryString);
                    }
                    break;

                case USEMTL:
                    /* Reset flag */
                    haveTexture = GL_FALSE;

                    /* Check if we have this texture */
                    for(i=0;i<object->materialCount;i++)
                    {
                        if(0 == strncmp(materials[i].name, token, strlen(materials[i].name))) 
                        {
                            /* We have this material already */
                            haveTexture = GL_TRUE;

                            /* Mark the start face that uses this texture */
                            object->materialChange[object->materialChangeCount].startFace = object->numOfFaces;
                            object->materialChange[object->materialChangeCount].startFace /= 9;
                            object->materialChange[object->materialChangeCount].material = &materials[i];
                            object->materialChangeCount++;
                            break;
                        }
                    }

                    /* If we dont have this texture name stored yet */
                    if ( GL_FALSE == haveTexture ) 
                    {
                        GLubyte cr = (strstr(token, "\r") == 0) ? 0 : 1;
                        GLubyte lf = (strstr(token, "\n") == 0) ? 0 : 1;

                        /* Add material name */
                        materials[object->materialCount].name = (char *)malloc(sizeof(GLbyte)*strlen(token));
                        memset(materials[object->materialCount].name, 0, sizeof(GLbyte)*strlen(token));
                        strncpy(materials[object->materialCount].name, token, strlen(token)-cr-lf);

                        /* Mark the start face that uses this texture */
                        object->materialChange[object->materialChangeCount].startFace = object->numOfFaces;
                        object->materialChange[object->materialChangeCount].startFace /= 9;
                        object->materialChange[object->materialChangeCount].material = &materials[object->materialCount];
                        object->materialChangeCount++;

                        /* Increment material count */
                        object->materialCount++;
                    }
                    break;

                default:  
                    break;
            }
        }


        /* Store the number of elements per entry */
        /* This will be stored each time, but the value for each element needs to be consistent anyways */
        if ( numOfElements != 0)
        {
            switch (entry)
            {
                case V:     object->numOfElementsPerVertices = numOfElements; break;
                case VT:    object->numOfElementsPerTexCoords = numOfElements; break;
                case VN:    object->numOfElementsPerNormals = numOfElements; break;
                case F:     object->numOfIndicesPerFace = numOfElements; break;
                default:    break;
            }
        }
    }
 
    object->numOfVertices /= (object->numOfElementsPerVertices) ? object->numOfElementsPerVertices : 1;
    object->numOfTexCoords /= (object->numOfElementsPerTexCoords) ? object->numOfElementsPerTexCoords : 1;
    object->numOfNormals /= (object->numOfElementsPerNormals) ? object->numOfElementsPerNormals : 1;
    object->numOfFaces /= (object->numOfIndicesPerFace) ? object->numOfIndicesPerFace : 1;
  
    fclose (pFile);

    return GL_TRUE;
}


void drawVertices(object_t *object, material_t *materials)
{
    GLint i;
    GLuint faceCount;
    
    glBindBuffer(GL_ARRAY_BUFFER, vboids[VBO_VERT]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*object->numOfFaces*3*3, object->vertArray, GL_STATIC_DRAW);
    glVertexAttribPointer(aVertexLoc, 3, GL_FLOAT, 0, 0, BUFFER_OFFSET(0));

    glBindBuffer(GL_ARRAY_BUFFER, vboids[VBO_TEX]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*object->numOfFaces*2*3, object->texArray, GL_STATIC_DRAW);
    glVertexAttribPointer(aTexCoordsLoc, 2, GL_FLOAT, 0, 0, BUFFER_OFFSET(0));

    glBindBuffer(GL_ARRAY_BUFFER, vboids[VBO_NORM]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*object->numOfFaces*3*3, object->normArray, GL_STATIC_DRAW);
    glVertexAttribPointer(aNormalLoc, 3, GL_FLOAT, 0, 0, BUFFER_OFFSET(0));

    glEnableVertexAttribArray(aVertexLoc);
    glEnableVertexAttribArray(aNormalLoc);
    glEnableVertexAttribArray(aTexCoordsLoc);

    for(i=0;i<object->materialChangeCount-1;i++)
    {
        glBindTexture(GL_TEXTURE_2D, object->materialChange[i].material->texId);
        faceCount = object->materialChange[i+1].startFace - object->materialChange[i].startFace;

        glDrawArrays(GL_TRIANGLES, object->materialChange[i].startFace*3, faceCount*3);
    }

    glBindTexture(GL_TEXTURE_2D, object->materialChange[i].material->texId);
    faceCount = (object->numOfFaces - object->materialChange[i].startFace+1);
    glDrawArrays(GL_TRIANGLES, object->materialChange[i].startFace*3, faceCount*3);


    glDisableVertexAttribArray(aVertexLoc);
    glDisableVertexAttribArray(aNormalLoc);
    glDisableVertexAttribArray(aTexCoordsLoc);
}



void prepareObjectArrays(object_t *object)
{
    GLint i;
    GLint v1 = 0, v2 = 0, v3 = 0;
    GLint vt1 = 0, vt2 = 0, vt3 = 0;
    GLint vn1 = 0, vn2 = 0, vn3 = 0;

    /* Assign pointers to input values */
    objVerts3_t *vertices = (objVerts3_t *)object->v;
    objVerts3_t *norms = (objVerts3_t *)object->vn;
    objTexCoords2_t *tex = (objTexCoords2_t *)object->vt;
    GLuint *faces = (GLuint *)object->f;

    /* Create vertex and texture buffers */
    object->vertArray = (GLfloat *)malloc(sizeof(GLfloat)*object->numOfFaces*3*3);
    object->texArray = (GLfloat *)malloc(sizeof(GLfloat)*object->numOfFaces*2*3);
    object->normArray = (GLfloat *)malloc(sizeof(GLfloat)*object->numOfFaces*3*3);
    
    GLuint vc = 0;
    GLuint tc = 0;
    GLuint vn = 0;

    GLuint stride = (object->numOfNormals == 0) ? 6 : 9;
    GLuint offset = (object->numOfNormals == 0) ? 0 : 1;

    for(i=0;i<object->numOfFaces;i++)
    {
        v1  = faces[(stride*i)+0]-1;
        vt1 = faces[(stride*i)+1]-1;
        v2  = faces[(stride*i)+2+offset]-1;
        vt2 = faces[(stride*i)+3+offset]-1;
        v3  = faces[(stride*i)+4+(offset*2)]-1;           
        vt3 = faces[(stride*i)+5+(offset*2)]-1;
        
        if(object->numOfNormals != 0)
        {
            vn1 = faces[(stride*i)+2]-1;
            vn2 = faces[(stride*i)+5+offset]-1;
            vn3 = faces[(stride*i)+8+(offset*2)]-1;
        }

        object->vertArray[vc++] = vertices[v1].x;
        object->vertArray[vc++] = vertices[v1].y;
        object->vertArray[vc++] = vertices[v1].z;
        
        object->vertArray[vc++] = vertices[v2].x;
        object->vertArray[vc++] = vertices[v2].y;
        object->vertArray[vc++] = vertices[v2].z;

        object->vertArray[vc++] = vertices[v3].x;
        object->vertArray[vc++] = vertices[v3].y;
        object->vertArray[vc++] = vertices[v3].z;

        if(object->numOfNormals != 0)
        {
            object->normArray[vn++] = norms[vn1].x;
            object->normArray[vn++] = norms[vn1].y;
            object->normArray[vn++] = norms[vn1].z;
            
            object->normArray[vn++] = norms[vn2].x;
            object->normArray[vn++] = norms[vn2].y;
            object->normArray[vn++] = norms[vn2].z;

            object->normArray[vn++] = norms[vn3].x;
            object->normArray[vn++] = norms[vn3].y;
            object->normArray[vn++] = norms[vn3].z;        
        }

        object->texArray[tc++] = tex[vt1].x;
        object->texArray[tc++] = tex[vt1].y;

        object->texArray[tc++] = tex[vt2].x;
        object->texArray[tc++] = tex[vt2].y;

        object->texArray[tc++] = tex[vt3].x;
        object->texArray[tc++] = tex[vt3].y;
    }
}


void cleanUp(object_t *object)
{
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


GLboolean loadModel(object_t *object, material_t *materials, char *objFileName, char *mtlFileName)
{
    GLint i;

    /* Load and parse obj file */
    printf("Loading object file: %s...", objFileName);

    if ( GL_FALSE == loadObjFile(object, materials, objFileName) )
    {
        cleanUp(object);
        return GL_FALSE;
    }
    printf("done\n");


    /* Load and parse material file */
    printf("Loading mtl file: %s...", mtlFileName);
    if ( GL_FALSE == loadMtlFile(object, materials, mtlFileName) )
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
            if ( GL_FALSE == loadTexture(&materials[i]) )
            {
                return GL_FALSE;
            }
            printf("done\n");
        }
    }

    return GL_TRUE;
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
    if (argc > 3)
    {
        cameraPosition.z = atof(argv[3]);
    }

    /* GL initialization */
    initGL();

    /* Load model */
    if ( GL_FALSE == loadModel(&object, materials, argv[1], argv[2]) )
    {
        printf("\nError loading model\n");
        return -1;
    }

    /* Prepare vertex and texture arrays */
    prepareObjectArrays(&object);

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

    cleanUp(&object);

    return 0;
}

