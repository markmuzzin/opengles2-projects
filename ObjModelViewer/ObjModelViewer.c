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

#define STRLEN                      128

#define DISPLAY_WIDTH               1024.0f
#define DISPLAY_HEIGHT              768.0f

#define BMP_WIDTH_OFFSET            18
#define BMP_HEIGHT_OFFSET           22
#define BMP_PIX_PER_COL             3
#define BMP_HEADER_SIZE             54

#define DEFAULT_CAM_DIST            3600.0f

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

/*******************************************************************/
/*  Typedefs                                                       */
/*******************************************************************/
typedef struct _model_t
{
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
} model_t;

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

typedef struct {
    GLuint v1;
    GLuint vt1;
    GLuint v2;
    GLuint vt2;
    GLuint v3;
    GLuint vt3;
} faces3_t;


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
static GLfloat *vertArray                = NULL;
static GLfloat *texArray                 = NULL;
static GLuint vboids[2]                  = {0};
static GLuint textureId                  = 0;

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

static GLint shaderProgram               = -1;
static GLint aVertexLoc                  = -1;
static GLint aTexCoordsLoc               = -1;

static GLint uTextureColorLoc            = -1;
static GLint uMVPLoc                     = -1;

static GLint appShutdown                 = 0;

static const GLchar* vertex_shader_source =    
{
    "precision mediump float;\n"

    "attribute vec2 aTexCoords;\n"
    "attribute vec4 aPosition;\n"

    "varying vec2 vTexCoords;\n"
    "varying vec4 vPosition;\n"

    "uniform mat4 uMVP;\n"

    "void main(void)\n"
    "{\n"
        "vTexCoords = aTexCoords;\n"
        "vPosition = vec4(aPosition.xyz, 1.0);"
        "gl_Position = vPosition * uMVP;\n"
    "}\n"
};


static const GLchar* fragment_shader_source =
{
    "precision mediump float;\n"
    "varying vec4 vPosition;\n"
    "varying vec2 vTexCoords;\n"

    "uniform sampler2D uTextureColor;\n"

    "void main(void)\n"
    "{\n"
        "vec4 color = texture2D(uTextureColor, vTexCoords);\n"
        "gl_FragColor = color.bgra;\n"
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
    uTextureColorLoc = glGetUniformLocation(shaderProgram, "uTextureColor");
    uMVPLoc = glGetUniformLocation(shaderProgram, "uMVP");

    /* Use program */
    glUseProgram(shaderProgram);

    /* Bind uniform samplers to texture units */
    glUniform1i(uTextureColorLoc, 0);
}


GLboolean loadTexture(const char *filename)
{
    GLubyte *data = NULL;
    FILE *file = NULL;

    /* Open File */
    file = fopen( filename, "rb" );
    if ( file == NULL ) 
    {   printf("error loading file\n");
        return GL_FALSE;
    }

    /* Read file */
    data = (GLubyte *)malloc( BMP_HEADER_SIZE );

    /* Read BMP Header */
    fread(data, BMP_HEADER_SIZE, 1, file);

    /* Get width and height from header */
    GLuint width = *(GLuint *)(&data[BMP_WIDTH_OFFSET]);
    GLuint height = *(GLuint *)(&data[BMP_HEIGHT_OFFSET]);

    /* Read pixel data */
    data = (GLubyte *)realloc( data, width * height * 3 );
    fread( data, width * height * BMP_PIX_PER_COL, 1, file );
    fclose( file );

    /* Generate and store texture */
    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);
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

    /* Generate VBO buffers */
    glGenBuffers(2, vboids);

    glfwSwapInterval(0);
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


GLboolean loadObjFile(char *fileName, model_t *model)
{
    FILE *pFile;
 	char line[STRLEN];
    char *token;
    char *subToken;
    char *lineString;
    char *entryString;
    GLuint numOfElements = 0;

    pFile = fopen(fileName, "r");
    if (pFile == NULL)
    {
       printf("Error opening OBJ file\n");
       return GL_FALSE;
    }
 
    while(fgets(line, STRLEN, pFile) != NULL )
    {
        GLubyte v  = (strncmp(line, "v ", 2) == 0) ? V  : 0;
        GLubyte vt = (strncmp(line, "vt", 2) == 0) ? VT : 0;
        GLubyte vn = (strncmp(line, "vn", 2) == 0) ? VN : 0;
        GLubyte f  = (strncmp(line, "f ", 2) == 0) ? F  : 0;

        GLubyte entry = v | vt | vn | f;
        numOfElements = 0;

        token = strtok_r(line, " ", &lineString);
                    
        while ((token = strtok_r(NULL, " ", &lineString)) != NULL )
        {   
            switch (entry)
            {
                case V:
                    addFloatData(&model->v, token, &model->numOfVertices); 
                    numOfElements++;
                    break;

                case VT:
                    addFloatData(&model->vt, token, &model->numOfTexCoords); 
                    numOfElements++;
                    break;

                case VN:
                    addFloatData(&model->vn, token, &model->numOfNormals); 
                    numOfElements++; 
                    break;

                case F:
                    while(token != NULL) 
                    {
                        subToken = strtok_r(token, "/", &entryString);

                        while (subToken != NULL) 
                        {
                            addIntegerData(&model->f, subToken, &model->numOfFaces); 
                            numOfElements++;
                            subToken = strtok_r(NULL, "/", &entryString);
                        }

                        token = strtok_r(NULL, " ", &entryString);
                    }
                    break;

                default:  
                    break;
            }
        }

        switch (entry)
        {
            case V:     model->numOfElementsPerVertices = numOfElements; break;
            case VT:    model->numOfElementsPerTexCoords = numOfElements; break;
            case VN:    model->numOfElementsPerNormals = numOfElements; break;
            case F:     model->numOfIndicesPerFace = numOfElements; break;
            default:    break;
        }      
    }
       
    fclose (pFile);
  
    model->numOfVertices /= (model->numOfElementsPerVertices) ? model->numOfElementsPerVertices : 1;
    model->numOfTexCoords /= (model->numOfElementsPerTexCoords) ? model->numOfElementsPerTexCoords : 1;
    model->numOfNormals /= (model->numOfElementsPerNormals) ? model->numOfElementsPerNormals : 1;
    model->numOfFaces /= (model->numOfIndicesPerFace) ? model->numOfIndicesPerFace : 1;
  
    return GL_TRUE;
}


void drawVertices(model_t model)
{
    glBindBuffer(GL_ARRAY_BUFFER, vboids[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*model.numOfFaces*2*3, texArray, GL_STATIC_DRAW);
    glVertexAttribPointer(aTexCoordsLoc, 2, GL_FLOAT, 0, 0, BUFFER_OFFSET(0));

    glBindBuffer(GL_ARRAY_BUFFER, vboids[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*model.numOfFaces*3*3, vertArray, GL_STATIC_DRAW);
    glVertexAttribPointer(aVertexLoc, 3, GL_FLOAT, 0, 0, BUFFER_OFFSET(0));

    glEnableVertexAttribArray(aVertexLoc);
    glEnableVertexAttribArray(aTexCoordsLoc);

    glDrawArrays(GL_TRIANGLES, 0, model.numOfFaces*3);

    glDisableVertexAttribArray(aVertexLoc);
    glDisableVertexAttribArray(aTexCoordsLoc);
}


void prepareModel(model_t model)
{
    GLint i, v1, v2, v3, vt1, vt2, vt3;

    objVerts3_t *vertices = (objVerts3_t *)model.v;
    faces3_t *faces = (faces3_t *)model.f;
    objTexCoords2_t *tex = (objTexCoords2_t *)model.vt;

    /* Create vertex and texture buffers */
    vertArray = (GLfloat *)malloc(sizeof(GLfloat)*model.numOfFaces*3*3);
    texArray = (GLfloat *)malloc(sizeof(GLfloat)*model.numOfFaces*2*3);
    
    GLuint vc = 0;
    GLuint tc = 0;

    /* Create vertex and texture arrays from face data */
    for(i=0;i<model.numOfFaces;i++)
    {
        v1=faces[i].v1-1;
        v2=faces[i].v2-1;
        v3=faces[i].v3-1;
        
        vt1=faces[i].vt1-1;
        vt2=faces[i].vt2-1;
        vt3=faces[i].vt3-1;

        vertArray[vc++] = vertices[v1].x;
        vertArray[vc++] = vertices[v1].y;
        vertArray[vc++] = vertices[v1].z;
        
        vertArray[vc++] = vertices[v2].x;
        vertArray[vc++] = vertices[v2].y;
        vertArray[vc++] = vertices[v2].z;

        vertArray[vc++] = vertices[v3].x;
        vertArray[vc++] = vertices[v3].y;
        vertArray[vc++] = vertices[v3].z;

        texArray[tc++] = tex[vt1].x;
        texArray[tc++] = tex[vt1].y;

        texArray[tc++] = tex[vt2].x;
        texArray[tc++] = tex[vt2].y;

        texArray[tc++] = tex[vt3].x;
        texArray[tc++] = tex[vt3].y;
    }
}


void cleanUp(void)
{
    if( vertArray != NULL )
    {
        free(vertArray);
    }

    if( texArray != NULL )
    {
        free(texArray);
    }
}


int main(int argc, char **argv)
{
    model_t model = { 0 };

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

    /* Load obj file */
    printf("Loading %s object file...", argv[1]);
    if ( GL_FALSE == loadObjFile(argv[1], &model) )
    {
        cleanUp();
        return -1;
    }
    printf("done\n");
    
    printf("Model vertices: %d\n", model.numOfVertices);
    printf("Model tex coords: %d\n", model.numOfTexCoords);
    printf("Model normals: %d\n", model.numOfNormals);
    printf("Model faces: %d\n", model.numOfFaces);

    printf("Loading %s texture file...", argv[2]);
    if ( GL_FALSE == loadTexture(argv[2]) )
    {
        cleanUp();
        return -2;
    }
    printf("done\n");
    
    /* Prepare vertex and texture arrays */
    prepareModel(model);

    /* Loop until we need to shutdown */
    while (!glfwWindowShouldClose(window) && appShutdown == 0) 
    {
        /* Check for events */
        glfwPollEvents();

        /* Check user input */
        checkUserInput();

        /* Clear the color and depth buffer */
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

        /* Draw the model */
        drawVertices(model);
    
        /* Rotate the model */
        modelRotationUp -= 2.0f;
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

    cleanUp();

    return 0;
}

