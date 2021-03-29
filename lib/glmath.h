#ifndef __GL_MATH_H__
#define __GL_MATH_H__

/*******************************************************************/
/*  Includes                                                       */
/*******************************************************************/
#define GLFW_INCLUDE_ES2
#include <GLFW/glfw3.h>
#include <GLES3/gl3.h>
#include <GLES2/gl2ext.h>

#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <stdio.h>

/*******************************************************************/
/*  Defines                                                        */
/*******************************************************************/
#define PI                          3.141592654f


/*******************************************************************/
/*  Typedefs                                                       */
/*******************************************************************/
typedef struct _vec4_t
{
    float x;
    float y;
    float z;
    float w;
} vec4_t;

typedef struct _vec3_t
{
    float x;
    float y;
    float z;
} vec3_t;


/*******************************************************************/
/*  Prototypes                                                     */
/*******************************************************************/
vec3_t normalize(vec3_t v);
vec3_t crossProd(vec3_t v, vec3_t u);
vec3_t scalarProd(vec3_t v, GLfloat u);
GLfloat dotProd(vec3_t v, vec3_t u);
vec3_t subProd(vec3_t v, vec3_t u);
vec3_t addProd(vec3_t v, vec3_t u);
void matrix4x4By4x4(float *src1, float *src2, float *dest);
void matrix4x4By4x1(float *src1, float *src2, float *dest);
void generateRotationMatrix(GLfloat angle, vec3_t axis, GLfloat *mat);
void generateScaleTranslationMatrix(vec3_t scale, vec3_t translate, GLfloat *mat);
void setIdentityMatrix(GLfloat* mat);
void generateLookAtMatrix(vec3_t eye, vec3_t target, vec3_t upDir, GLfloat *mat);
void generatePerspectiveProjectionMatrix(GLfloat fov, GLfloat aspect, GLfloat zNear, GLfloat zFar, GLfloat *mat);

#endif

