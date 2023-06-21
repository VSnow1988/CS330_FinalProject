#include <iostream>             // cout, cerr
#include <cstdlib>              // EXIT_FAILURE
#include <GL/glew.h>            // GLEW library
#include <GLFW/glfw3.h>         // GLFW library
#include "camera.h" // Camera class
#include "meshes.h" // Basic shape meshes
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// GLM Math Header inclusions
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace std; // Standard namespace

/*Shader program Macro*/
#ifndef GLSL
#define GLSL(Version, Source) "#version " #Version " core \n" #Source
#endif

// Unnamed namespace
namespace
{
    const char* const WINDOW_TITLE = "Vincent Snow - CS 330 Project"; // Macro for window title
    bool gOrtho = false;

    // Variables for window width and height
    const int WINDOW_WIDTH = 800;
    const int WINDOW_HEIGHT = 800;

    // Stores the GL data relative to a given mesh
    struct GLMesh
    {
        GLuint vao;         // Handle for the vertex array object
        GLuint vbo[2];         // Handles for the vertex buffer object
        GLuint nVertices;    // Number of indices of the mesh
        GLuint nIndices;
    };

    // Main GLFW window
    GLFWwindow* gWindow = nullptr;
    // Use meshes from the included Meshes.cpp file
    Meshes meshes;
    // Shader programs
    GLuint gProgramId;
    GLuint gLightProgramId;
    // Textures
    GLuint gTextureIdTwine;
    GLuint gTextureIdWoodtable;
    GLuint gTextureIdWoodsticks;
    GLuint gTextureIdAmethyst;
    GLuint gTextureIdRedMarble;
    GLuint gTextureIdCandle;
    GLuint gTextureIdMetal;
    GLuint gTextureIdGlass;
    glm::vec2 gUVScale(1.0f, 1.0f); // Scale of the textures
    GLint gTexWrapMode = GL_REPEAT; // Tile the textures
    // camera
    Camera gCamera(glm::vec3(0.0f, 0.0f, 3.0f));
    float gLastX = WINDOW_WIDTH / 2.0f;
    float gLastY = WINDOW_HEIGHT / 2.0f;
    bool gFirstMouse = true;

    // timing
    float gDeltaTime = 0.0f; // time between current frame and last frame
    float gLastFrame = 0.0f;

}

int main(int argc, char* argv[]);

/* User-defined Function prototypes to:
 * initialize the program, set the window size,
 * redraw graphics on the window when resized,
 * and render graphics on the screen
 */
bool UInitialize(int, char* [], GLFWwindow** window);
void UResizeWindow(GLFWwindow* window, int width, int height);
void UProcessInput(GLFWwindow* window);
void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos);
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void UMouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void URender();
bool UCreateTexture(const char* filename, GLuint& textureId);
void UDestroyTexture(GLuint textureId);
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId);
void UDestroyShaderProgram(GLuint programId);


/* Vertex Shader Source Code*/
const GLchar* vertexShaderSource = GLSL(440,
    layout(location = 0) in vec3 vertexPosition; // VAP position 0 for vertex position data
layout(location = 1) in vec3 vertexNormal; // VAP position 1 for normals
layout(location = 2) in vec2 textureCoordinate;
layout(location = 3) in vec4 color;  // Color data from Vertex Attrib Pointer 1

out vec2 vertexTextureCoordinate; // transfer texture data to fragment shader
out vec4 vertexColor; // variable to transfer color data to the fragment shader
out vec3 vertexFragmentNormal; // For outgoing normals to fragment shader
out vec3 vertexFragmentPos; // For outgoing color / pixels to fragment shader

//Global variables for the  transform matrices
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(vertexPosition, 1.0f); // transforms vertices to clip coordinates
    vertexColor = color; // references incoming color data
    vertexTextureCoordinate = textureCoordinate; // references texture data
    vertexFragmentPos = vec3(model * vec4(vertexPosition, 1.0f)); // Gets fragment / pixel position in world space only (exclude view and projection)
    vertexFragmentNormal = mat3(transpose(inverse(model))) * vertexNormal; // get normal vectors in world space only and exclude normal translation properties
}
);


/* Fragment Shader Source Code*/
const GLchar* fragmentShaderSource = GLSL(440,
    in vec4 vertexColor; // Variable to hold incoming color data from vertex shader
    in vec3 vertexFragmentNormal; // For incoming normals
in vec3 vertexFragmentPos; // For incoming fragment position
in vec2 vertexTextureCoordinate; // Variable to hold texture data
out vec4 fragmentColor;

//Uniform variables
uniform vec4 objectColor;
uniform vec3 ambientColor;
uniform vec3 light1Color;
uniform vec3 light1Position;
uniform vec3 light2Color;
uniform vec3 light2Position;
uniform vec3 viewPosition;
uniform sampler2D uTexture; // Useful when working with multiple textures
uniform vec2 uvScale;
uniform bool ubHasTexture;
uniform float ambientStrength = 1.0f; // Set ambient or global lighting strength
uniform float specularIntensity1 = 1.0f; // Front light
uniform float highlightSize1 = 16.0f;
uniform float specularIntensity2 = 1.0f; // Back light
uniform float highlightSize2 = 16.0f;

void main()
{
    /*Phong lighting model calculations to generate ambient, diffuse, and specular components*/

    //Calculate Ambient lighting
    vec3 ambient = ambientStrength * ambientColor; // Generate ambient light color

    //**Calculate Diffuse lighting**
    vec3 norm = normalize(vertexFragmentNormal); // Normalize vectors to 1 unit
    vec3 light1Direction = normalize(light1Position - vertexFragmentPos); // Calculate distance (light direction) between light source and fragments/pixels on cube
    float impact1 = max(dot(norm, light1Direction), 0.0);// Calculate diffuse impact by generating dot product of normal and light
    vec3 diffuse1 = impact1 * light1Color; // Generate diffuse light color
    vec3 light2Direction = normalize(light2Position - vertexFragmentPos); // Calculate distance (light direction) between light source and fragments/pixels on cube
    float impact2 = max(dot(norm, light2Direction), 0.0);// Calculate diffuse impact by generating dot product of normal and light
    vec3 diffuse2 = impact2 * light2Color; // Generate diffuse light color

    //**Calculate Specular lighting**
    vec3 viewDir = normalize(viewPosition - vertexFragmentPos); // Calculate view direction
    vec3 reflectDir1 = reflect(-light1Direction, norm);// Calculate reflection vector
    //Calculate specular component
    float specularComponent1 = pow(max(dot(viewDir, reflectDir1), 0.0), highlightSize1);
    vec3 specular1 = specularIntensity1 * specularComponent1 * light1Color;
    vec3 reflectDir2 = reflect(-light2Direction, norm);// Calculate reflection vector
    //Calculate specular component
    float specularComponent2 = pow(max(dot(viewDir, reflectDir2), 0.0), highlightSize2);
    vec3 specular2 = specularIntensity2 * specularComponent2 * light2Color;

    //**Calculate phong result**
    //Texture holds the color to be used for all three components
    vec4 textureColor = texture(uTexture, vertexTextureCoordinate * uvScale);
    vec3 phong1;
    vec3 phong2;

    if (ubHasTexture == true)
    {
        phong1 = (ambient + diffuse1 + specular1) * textureColor.xyz;
        phong2 = (ambient + diffuse2 + specular2) * textureColor.xyz;
    }
    else
    {
        phong1 = (ambient + diffuse1 + specular1) * objectColor.xyz;
        phong2 = (ambient + diffuse2 + specular2) * objectColor.xyz;
    }

    fragmentColor = vec4(phong1 + phong2, 1.0); // Send lighting results to GPU
    //fragmentColor = texture(uTexture, vertexTextureCoordinate); // Sends texture to the GPU for rendering
}
);

/* Light Object Shader Source Code*/
const GLchar* lightVertexShaderSource = GLSL(330,
    layout(location = 0) in vec3 aPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
);

/* Light Object Shader Source Code*/
const GLchar* lightFragmentShaderSource = GLSL(330,
    out vec4 FragColor;

void main()
{
    FragColor = vec4(1.0); // set all 4 vector values to 1.0
}
);

// Images are loaded with Y axis going down, but OpenGL's Y axis goes up, so let's flip it
void flipImageVertically(unsigned char* image, int width, int height, int channels)
{
    for (int j = 0; j < height / 2; ++j)
    {
        int index1 = j * width * channels;
        int index2 = (height - 1 - j) * width * channels;

        for (int i = width * channels; i > 0; --i)
        {
            unsigned char tmp = image[index1];
            image[index1] = image[index2];
            image[index2] = tmp;
            ++index1;
            ++index2;
        }
    }
}

// Main function for OpenGL Program
int main(int argc, char* argv[])
{
    if (!UInitialize(argc, argv, &gWindow))
        return EXIT_FAILURE;

    // Create the mesh
    meshes.CreateMeshes(); // Calls the function to create the Vertex Buffer Object

    // Create the shader program
    if (!UCreateShaderProgram(vertexShaderSource, fragmentShaderSource, gProgramId))
        return EXIT_FAILURE;

    if (!UCreateShaderProgram(lightVertexShaderSource, lightFragmentShaderSource, gLightProgramId))
        return EXIT_FAILURE;

    // Load textures (relative to exe file's directory)
    const char* twine_tex = "../resources/textures/twine_tex.jpg";
    if (!UCreateTexture(twine_tex, gTextureIdTwine))
    {
        cout << "Failed to load texture " << twine_tex << endl;
        return EXIT_FAILURE;
    }
    const char* table_tex = "../resources/textures/woodtable.jpg";
    if (!UCreateTexture(table_tex, gTextureIdWoodtable))
    {
        cout << "Failed to load texture " << table_tex << endl;
        return EXIT_FAILURE;
    }
    const char* sticks_tex = "../resources/textures/woodsticks.jpg";
    if (!UCreateTexture(sticks_tex, gTextureIdWoodsticks))
    {
        cout << "Failed to load texture " << sticks_tex << endl;
        return EXIT_FAILURE;
    }
    const char* pyramid_tex = "../resources/textures/amethyst_tex.jpg";
    if (!UCreateTexture(pyramid_tex, gTextureIdAmethyst))
    {
        cout << "Failed to load texture " << pyramid_tex << endl;
        return EXIT_FAILURE;
    }
    const char* marble_tex = "../resources/textures/red_tex.jpg";
    if (!UCreateTexture(marble_tex, gTextureIdRedMarble))
    {
        cout << "Failed to load texture " << marble_tex << endl;
        return EXIT_FAILURE;
    }
    const char* candle_tex = "../resources/textures/candle_tex.png";
    if (!UCreateTexture(candle_tex, gTextureIdCandle))
    {
        cout << "Failed to load texture " << candle_tex << endl;
        return EXIT_FAILURE;
    }
    const char* metal_tex = "../resources/textures/metal_tex.jpg";
    if (!UCreateTexture(metal_tex, gTextureIdMetal))
    {
        cout << "Failed to load texture " << metal_tex << endl;
        return EXIT_FAILURE;
    }
    const char* glass_tex = "../resources/textures/glass_tex.jpg";
    if (!UCreateTexture(glass_tex, gTextureIdGlass))
    {
        cout << "Failed to load texture " << glass_tex << endl;
        return EXIT_FAILURE;
    }

    // tell opengl for each sampler to which texture unit it belongs to (only has to be done once)
    glUseProgram(gProgramId);
    // We set the texture as texture unit 0
    glUniform1i(glGetUniformLocation(gProgramId, "uTexture"), 0);
    glUniform2f(glGetUniformLocation(gProgramId, "uvScale"), gUVScale.x, gUVScale.y);

    // Sets the background color of the window to black (it will be implicitely used by glClear)
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // Camera parameters
    gCamera.Position = glm::vec3(0.0f, 3.0f, 6.0f);
    gCamera.Front = glm::vec3(0.0f, -1.0f, -2.0f);
    gCamera.Up = glm::vec3(0.0f, 1.0f, 0.0f);

    // render loop
    // -----------
    while (!glfwWindowShouldClose(gWindow))
    {
        // per-frame timing
        // --------------------
        float currentFrame = glfwGetTime();
        gDeltaTime = currentFrame - gLastFrame;
        gLastFrame = currentFrame;

        // input
        // -----
        UProcessInput(gWindow);

        // Render this frame
        URender();

        glfwPollEvents();
    }

    // Release mesh data
    Meshes().DestroyMeshes();

    // Release texture
    UDestroyTexture(gTextureIdTwine);
    UDestroyTexture(gTextureIdWoodtable);
    UDestroyTexture(gTextureIdWoodsticks);
    UDestroyTexture(gTextureIdAmethyst);
    UDestroyTexture(gTextureIdRedMarble);
    UDestroyTexture(gTextureIdCandle);
    UDestroyTexture(gTextureIdMetal);
    UDestroyTexture(gTextureIdGlass);

    // Release shader program
    UDestroyShaderProgram(gProgramId);
    UDestroyShaderProgram(gLightProgramId);

    exit(EXIT_SUCCESS); // Terminates the program successfully
}


// Initialize GLFW, GLEW, and create a window
bool UInitialize(int argc, char* argv[], GLFWwindow** window)
{
    // GLFW: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // GLFW: window creation
    // ---------------------
    * window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, NULL, NULL);
    if (*window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }
    glfwMakeContextCurrent(*window);
    glfwSetFramebufferSizeCallback(*window, UResizeWindow);
    glfwSetCursorPosCallback(*window, UMousePositionCallback);
    glfwSetScrollCallback(*window, UMouseScrollCallback);
    glfwSetMouseButtonCallback(*window, UMouseButtonCallback);

    // tell GLFW to capture our mouse
    glfwSetInputMode(*window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // GLEW: initialize
    // ----------------
    // Note: if using GLEW version 1.13 or earlier
    glewExperimental = GL_TRUE;
    GLenum GlewInitResult = glewInit();

    if (GLEW_OK != GlewInitResult)
    {
        std::cerr << glewGetErrorString(GlewInitResult) << std::endl;
        return false;
    }

    // Displays GPU OpenGL version
    cout << "INFO: OpenGL Version: " << glGetString(GL_VERSION) << endl;

    return true;
}


// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
void UProcessInput(GLFWwindow* window)
{
    static const float cameraSpeed = 2.5f;
    // Exit program
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    // Movement
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        gCamera.ProcessKeyboard(FORWARD, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        gCamera.ProcessKeyboard(BACKWARD, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        gCamera.ProcessKeyboard(LEFT, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        gCamera.ProcessKeyboard(RIGHT, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        gCamera.ProcessKeyboard(UP, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        gCamera.ProcessKeyboard(DOWN, gDeltaTime);

    // View toggles
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS)
    {
        gOrtho = false;
        gCamera.Position = glm::vec3(0.0f, 3.0f, 6.0f);
        gCamera.Front = glm::vec3(0.0f, -1.0f, -2.0f);
        gCamera.Up = glm::vec3(0.0f, 1.0f, 0.0f);
        }

    if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS)
    {
        gOrtho = true;
        gCamera.Position = glm::vec3(0.0f, 0.0f, 6.0f);
        gCamera.Front = glm::vec3(0.0f, 0.0f, -2.0f);
        gCamera.Up = glm::vec3(0.0f, 1.0f, 0.0f);
    }
}


// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void UResizeWindow(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}


// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos)
{
    if (gOrtho == true) {
        return;
    }
    if (gFirstMouse)
    {
        gLastX = xpos;
        gLastY = ypos;
        gFirstMouse = false;
    }

    float xoffset = xpos - gLastX;
    float yoffset = gLastY - ypos; // reversed since y-coordinates go from bottom to top

    gLastX = xpos;
    gLastY = ypos;

    gCamera.ProcessMouseMovement(xoffset, yoffset);
}


// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    gCamera.ProcessMouseScroll(yoffset);
}

// glfw: handle mouse button events
// --------------------------------
void UMouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    switch (button)
    {
    case GLFW_MOUSE_BUTTON_LEFT:
    {
        if (action == GLFW_PRESS)
            cout << "Left mouse button pressed" << endl;
        else
            cout << "Left mouse button released" << endl;
    }
    break;

    case GLFW_MOUSE_BUTTON_MIDDLE:
    {
        if (action == GLFW_PRESS)
            cout << "Middle mouse button pressed" << endl;
        else
            cout << "Middle mouse button released" << endl;
    }
    break;

    case GLFW_MOUSE_BUTTON_RIGHT:
    {
        if (action == GLFW_PRESS)
            cout << "Right mouse button pressed" << endl;
        else
            cout << "Right mouse button released" << endl;
    }
    break;

    default:
        cout << "Unhandled mouse button event" << endl;
        break;
    }
}


// Function called to render a frame
void URender()
{
    //Declarations of varaibles
    GLint modelLoc;
    GLint viewLoc;
    GLint projLoc;
    GLint viewPosLoc;
    GLint ambStrLoc;
    GLint ambColLoc;
    GLint light1ColLoc;
    GLint light1PosLoc;
    GLint light2ColLoc;
    GLint light2PosLoc;
    GLint objColLoc;
    GLint specInt1Loc;
    GLint highlghtSz1Loc;
    GLint specInt2Loc;
    GLint highlghtSz2Loc;
    GLint uHasTextureLoc;
    bool ubHasTextureVal;
    glm::mat4 scale;
    glm::mat4 rotation;
    glm::mat4 translation;
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 projection;

    const int nrows = 10;
    const int ncols = 10;
    const int nlevels = 10;

    const float xsize = 10.0f;
    const float ysize = 10.0f;
    const float zsize = 10.0f;

    // Enable z-depth
    glEnable(GL_DEPTH_TEST);

    // Clear the frame and z buffers
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // camera/view transformation
    view = gCamera.GetViewMatrix();

    // Creates a perspective projection
    if (gOrtho == false) {
        projection = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);
    }
    else {
        projection = glm::ortho(-5.0f, 5.0f, -5.0f, 5.0f, 0.1f, 100.0f);
    }

    // Set the shader to be used
    glUseProgram(gProgramId);

    // Retrieves and passes transform matrices to the Shader program
    modelLoc = glGetUniformLocation(gProgramId, "model");
    viewLoc = glGetUniformLocation(gProgramId, "view");
    projLoc = glGetUniformLocation(gProgramId, "projection");
    viewPosLoc = glGetUniformLocation(gProgramId, "viewPosition");
    ambStrLoc = glGetUniformLocation(gProgramId, "ambientStrength");
    ambColLoc = glGetUniformLocation(gProgramId, "ambientColor");
    light1ColLoc = glGetUniformLocation(gProgramId, "light1Color");
    light1PosLoc = glGetUniformLocation(gProgramId, "light1Position");
    light2ColLoc = glGetUniformLocation(gProgramId, "light2Color");
    light2PosLoc = glGetUniformLocation(gProgramId, "light2Position");
    objColLoc = glGetUniformLocation(gProgramId, "objectColor");
    specInt1Loc = glGetUniformLocation(gProgramId, "specularIntensity1");
    highlghtSz1Loc = glGetUniformLocation(gProgramId, "highlightSize1");
    specInt2Loc = glGetUniformLocation(gProgramId, "specularIntensity2");
    highlghtSz2Loc = glGetUniformLocation(gProgramId, "highlightSize2");
    uHasTextureLoc = glGetUniformLocation(gProgramId, "ubHasTexture");

    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    //set ambient lighting strength
    glUniform1f(ambStrLoc, 0.5f);
    //set ambient color
    glUniform3f(ambColLoc, 1.0f, 0.9f, 0.8f); // Warm sunlight ambience
    glUniform3f(light1ColLoc, 1.0f, 0.9f, 0.5f); // Front light - warm
    glUniform3f(light1PosLoc, -3.0f, 7.0f, 5.0f);
    glUniform3f(light2ColLoc, 1.0f, 0.9f, 0.5f); // Back light - warm
    glUniform3f(light2PosLoc, 3.0f, 7.0f, -5.0f);
    //set specular intensity
    glUniform1f(specInt1Loc, 0.2f); // front light
    glUniform1f(specInt2Loc, 0.2f); // back light
    //set specular highlight size
    glUniform1f(highlghtSz1Loc, 2.0f);
    glUniform1f(highlghtSz2Loc, 2.0f);

    ubHasTextureVal = true;
    glUniform1i(uHasTextureLoc, ubHasTextureVal);

    // Activate mesh for palo santo sticks
    glBindVertexArray(meshes.gBoxMesh.vao);

    // bind textures on corresponding texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gTextureIdWoodsticks);

    // Transformation for the bottom palo santo stick
    // 1. Scales the cubes into a thin rectangle.
    scale = glm::scale(glm::vec3(0.6f, 0.4f, 3.0f));
    // 2. Rotates shape 
    glm::mat4 rotationy = glm::rotate(glm::radians(-90.0f), glm::vec3(0.0, 1.0f, 0.0f));
    glm::mat4 rotationz = glm::rotate(glm::radians(30.0f), glm::vec3(0.0, 0.0f, 1.0f));
    glm::mat4 rotationx = glm::rotate(glm::radians(-95.0f), glm::vec3(1.0, 0.0f, 0.0f));
    // 3. Place object in lower right area
    translation = glm::translate(glm::vec3(1.2f, -3.2f, 2.9f));
    // Model matrix: transformations are applied right-to-left order
    model = translation * rotationx * rotationz * rotationy * scale;
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    // Draws the triangles
    glDrawElements(GL_TRIANGLES, meshes.gBoxMesh.nIndices, GL_UNSIGNED_INT, (void*)0);

    // Transformations for top palo santo stick
    rotationy = glm::rotate(glm::radians(-90.0f), glm::vec3(0.0, 1.0f, 0.0f));
    rotationz = glm::rotate(glm::radians(25.0f), glm::vec3(0.0, 0.0f, 1.0f));
    rotationx = glm::rotate(glm::radians(-75.0f), glm::vec3(1.0, 0.0f, 0.0f));
    translation = glm::translate(glm::vec3(1.3f, -3.2f, 2.5f));
    // Model matrix: transformations are applied right-to-left order
    model = translation * rotationx * rotationz * rotationy * scale;
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    // Draws the triangles
    glDrawElements(GL_TRIANGLES, meshes.gBoxMesh.nIndices, GL_UNSIGNED_INT, (void*)0);

    // Deactivate the Vertex Array Object and textures
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Bind and start creating torus (twine)
    glBindVertexArray(meshes.gTorusMesh.vao);

    // Transformation for the torus mesh (twine)
    // 1. Scales into a thin torus
    scale = glm::scale(glm::vec3(0.6f, 0.5f, 1.3f));
    // 2. Rotates shape 
    rotationy = glm::rotate(glm::radians(100.0f), glm::vec3(0.0, 1.0f, 0.0f));
    rotationz = glm::rotate(glm::radians(15.0f), glm::vec3(0.0, 0.0f, 1.0f));
    rotationx = glm::rotate(glm::radians(-45.0f), glm::vec3(1.0, 0.0f, 0.0f));
    // 3. Place object 
    translation = glm::translate(glm::vec3(1.1f, -3.25f, 2.8f));
    // Model matrix: transformations are applied right-to-left order
    model = translation * rotationx * rotationy * rotationz * scale;
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    // bind textures on corresponding texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gTextureIdTwine);

    // Draws the triangles
    glDrawArrays(GL_TRIANGLES, 0, meshes.gTorusMesh.nVertices);

    // Deactivate the Vertex Array Object
    glBindVertexArray(0);

    // Bind and start creating cylinder (twine ends)
    glBindVertexArray(meshes.gCylinderMesh.vao);

    // Transformation for the cylinder mesh (twine ends)
    // 1. Scales into a very thin cylinder
    scale = glm::scale(glm::vec3(0.05f, 1.5f, 0.05f));
    // 2. Rotates shape
    rotationx = glm::rotate(glm::radians(15.0f), glm::vec3(1.0, 0.0f, 0.0f));
    rotationy = glm::rotate(glm::radians(-110.0f), glm::vec3(1.0, 0.0f, 0.0f));
    // 3. Place object 
    translation = glm::translate(glm::vec3(1.2f, -2.7f, 3.6f));
    // Model matrix: transformations are applied right-to-left order
    model = translation * rotationx * rotationy * scale;
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    // bind textures on corresponding texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gTextureIdTwine);

    // Draws the triangles
    glDrawArrays(GL_TRIANGLE_FAN, 0, 36);		//bottom
    glDrawArrays(GL_TRIANGLE_FAN, 36, 36);		//top
    glDrawArrays(GL_TRIANGLE_STRIP, 72, 146);	//sides

    // Deactivate the Vertex Array Object
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Bind and start creating Orgone Pyramid
    glBindVertexArray(meshes.gPyramid4Mesh.vao);

    // Transformations for pyramid
    // 1. Scale
    scale = glm::scale(glm::vec3(2.0f, 1.0f, 2.0f));
    // 2. Rotate
    rotationy = glm::rotate(glm::radians(85.0f), glm::vec3(0.0, 1.0f, 0.0f));
    // 3. Place 
    translation = glm::translate(glm::vec3(-2.0f, -3.0f, 3.0f));
    // Model matrix: transformations are applied right-to-left order
    model = translation * rotationy * scale;
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    // bind textures on corresponding texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gTextureIdAmethyst);

    // Draws the triangles
    glDrawArrays(GL_TRIANGLE_STRIP, 0, meshes.gPyramid4Mesh.nVertices);

    // Deactivate the Vertex Array Object and texture
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Bind and start creating Red Onyx Marble
    glBindVertexArray(meshes.gSphereMesh.vao);

    // Transformations for sphere
    // 1. Scale
    scale = glm::scale(glm::vec3(0.3f, 0.3f, 0.3f));
    // 2. Rotate
    rotationx = glm::rotate(glm::radians(0.0f), glm::vec3(1.0, 0.0f, 0.0f));
    // 3. Place 
    translation = glm::translate(glm::vec3(1.5f, -3.3f, 0.0f));
    // Model matrix: transformations are applied right-to-left order
    model = translation * rotationx * scale;
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    // bind textures on corresponding texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gTextureIdRedMarble);

    // Draws the triangles
    glDrawElements(GL_TRIANGLES, meshes.gSphereMesh.nIndices, GL_UNSIGNED_INT, (void*)0);

    // Deactivate the Vertex Array Object and texture
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Bind and start creating metal tip
    glBindVertexArray(meshes.gConeMesh.vao);

    // Transformations for cone
    // 1. Scale
    scale = glm::scale(glm::vec3(0.1f, 0.1f, 0.1f));
    // 2. Rotate
    rotationx = glm::rotate(glm::radians(90.0f), glm::vec3(1.0, 0.0f, 0.0f));
    // 3. Place 
    translation = glm::translate(glm::vec3(1.5f, -3.3f, 0.25f));
    // Model matrix: transformations are applied right-to-left order
    model = translation * rotationx * scale;
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    // bind textures on corresponding texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gTextureIdMetal);

    // Draws the triangles
    glDrawArrays(GL_TRIANGLE_FAN, 0, 36);		//bottom
    glDrawArrays(GL_TRIANGLE_STRIP, 36, 108);	//sides

    // Deactivate the Vertex Array Object and texture
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Bind and start creating chain
    glBindVertexArray(meshes.gCylinderMesh.vao);

    // bind textures on corresponding texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gTextureIdMetal);

    // Transformations for chain cylinder
    // 1. Scale
    scale = glm::scale(glm::vec3(0.02f, 1.2f, 0.02f));
    // 2. Rotate
    rotationx = glm::rotate(glm::radians(-105.0f), glm::vec3(1.0, 0.0f, 0.0f));
    rotationz = glm::rotate(glm::radians(-40.0f), glm::vec3(0.0, 0.0f, 1.0f));
    // 3. Place 
    translation = glm::translate(glm::vec3(1.5f, -3.3f, -0.5f));
    // Model matrix: transformations are applied right-to-left order
    model = translation * rotationx * rotationz * scale;
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    // Draws the triangles
    glDrawArrays(GL_TRIANGLE_FAN, 0, 36);		//bottom
    glDrawArrays(GL_TRIANGLE_FAN, 36, 36);		//top
    glDrawArrays(GL_TRIANGLE_STRIP, 72, 146);	//sides

    //Draw the right part of chain
    // 1. Scale
    scale = glm::scale(glm::vec3(0.02f, 1.2f, 0.02f));
    // 2. Rotate
    rotationx = glm::rotate(glm::radians(-90.0f), glm::vec3(1.0, 0.0f, 0.0f)); //x relative to scene
    rotationz = glm::rotate(glm::radians(30.0f), glm::vec3(0.0, 0.0f, 1.0f)); //y relative to scene
    // 3. Place 
    translation = glm::translate(glm::vec3(2.86f, -3.53f, -0.35f));
    // Model matrix: transformations are applied right-to-left order
    model = translation * rotationx * rotationz * scale;
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    // Draws the triangles
    glDrawArrays(GL_TRIANGLE_FAN, 0, 36);		//bottom
    glDrawArrays(GL_TRIANGLE_FAN, 36, 36);		//top
    glDrawArrays(GL_TRIANGLE_STRIP, 72, 146);	//sides

    // Transformations for ring stump
    // 1. Scale
    scale = glm::scale(glm::vec3(0.05f, 0.1f, 0.05f));
    // 2. Rotate
    rotationx = glm::rotate(glm::radians(90.0f), glm::vec3(1.0, 0.0f, 0.0f));
    rotationz = glm::rotate(glm::radians(0.0f), glm::vec3(0.0, 0.0f, 1.0f));
    // 3. Place 
    translation = glm::translate(glm::vec3(1.5f, -3.3f, -0.4f));
    // Model matrix: transformations are applied right-to-left order
    model = translation * rotationx * rotationz * scale;
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    // bind textures on corresponding texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gTextureIdMetal);

    // Draws the triangles
    glDrawArrays(GL_TRIANGLE_FAN, 0, 36);		//bottom
    glDrawArrays(GL_TRIANGLE_FAN, 36, 36);		//top
    glDrawArrays(GL_TRIANGLE_STRIP, 72, 146);	//sides

    // Deactivate the Vertex Array Object and texture
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Bind and start creating chain loop
    glBindVertexArray(meshes.gTorusMesh.vao);

    // Transformations for torus
    // 1. Scale
    scale = glm::scale(glm::vec3(0.08f, 0.08f, 0.08f));
    // 2. Rotate
    rotationx = glm::rotate(glm::radians(90.0f), glm::vec3(1.0, 0.0f, 0.0f));
    // 3. Place 
    translation = glm::translate(glm::vec3(1.5f, -3.3f, -0.45f));
    // Model matrix: transformations are applied right-to-left order
    model = translation * rotationx * scale;
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    // bind textures on corresponding texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gTextureIdMetal);

    // Draws the triangles
    glDrawArrays(GL_TRIANGLES, 0, meshes.gTorusMesh.nVertices);

    // Deactivate the Vertex Array Object and texture
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Bind and start creating Small Bead
    glBindVertexArray(meshes.gSphereMesh.vao);

    // Transformations for sphere
    // 1. Scale
    scale = glm::scale(glm::vec3(0.1f, 0.1f, 0.1f));
    // 2. Rotate
    rotationx = glm::rotate(glm::radians(0.0f), glm::vec3(1.0, 0.0f, 0.0f));
    // 3. Place 
    translation = glm::translate(glm::vec3(2.9f, -3.5f, -0.3f));
    // Model matrix: transformations are applied right-to-left order
    model = translation * rotationx * scale;
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    // bind textures on corresponding texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gTextureIdGlass);

    // Draws the triangles
    glDrawElements(GL_TRIANGLES, meshes.gSphereMesh.nIndices, GL_UNSIGNED_INT, (void*)0);

    // Deactivate the Vertex Array Object and texture
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Bind and start creating candle
    glBindVertexArray(meshes.gCylinderMesh.vao);

    // Transformations for cylinder
    // 1. Scale
    scale = glm::scale(glm::vec3(1.5f, 2.0f, 1.5f));
    // 2. Rotate
    rotationx = glm::rotate(glm::radians(0.0f), glm::vec3(1.0, 0.0f, 0.0f));
    // 3. Place 
    translation = glm::translate(glm::vec3(-1.5f, -3.5f, -0.5f));
    // Model matrix: transformations are applied right-to-left order
    model = translation * rotationx * scale;
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    // bind textures on corresponding texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gTextureIdCandle);

    // Draws the triangles
    glDrawArrays(GL_TRIANGLE_FAN, 0, 36);		//bottom
    glDrawArrays(GL_TRIANGLE_FAN, 36, 36);		//top
    glDrawArrays(GL_TRIANGLE_STRIP, 72, 146);	//sides

    // Deactivate the Vertex Array Object and texture
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Bind and start creating plane (wood table)
    glBindVertexArray(meshes.gPlaneMesh.vao);

    // Transformations for plane
    // 1. Scale
    scale = glm::scale(glm::vec3(6.0f, 10.0f, 6.0f));
    // 2. Rotate
    rotationx = glm::rotate(glm::radians(0.0f), glm::vec3(1.0, 0.0f, 0.0f));
    // 3. Place 
    translation = glm::translate(glm::vec3(0.0f, -3.6f, 0.0f));
    // Model matrix: transformations are applied right-to-left order
    model = translation * rotationx * scale;
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    // bind textures on corresponding texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gTextureIdWoodtable);

    // Draws the triangles
    glDrawElements(GL_TRIANGLES, meshes.gPlaneMesh.nIndices, GL_UNSIGNED_INT, (void*)0);

    // Deactivate the Vertex Array Object and texture
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);

    /* CREATE LIGHT OBJECTS AND SOURCES */
    // Set the shader to be used
    glUseProgram(gLightProgramId);

    // Retrieves and passes transform matrices to the Shader program
    modelLoc = glGetUniformLocation(gLightProgramId, "model");
    viewLoc = glGetUniformLocation(gLightProgramId, "view");
    projLoc = glGetUniformLocation(gLightProgramId, "projection");

    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // Front light: Cool, low intensity
    // Activate the VBOs contained within the mesh's VAO
    glBindVertexArray(meshes.gTorusMesh.vao);

    // 1. Scales the object 
    scale = glm::scale(glm::vec3(1.0f, 1.0f, 1.0f));
    // 2. Rotates shape over the x axis
    rotation = glm::rotate(glm::radians(90.0f), glm::vec3(1.0, 0.0f, 0.0f));
    // 3. Place object
    translation = glm::translate(glm::vec3(-3.0f, 7.0f, 5.0f));
    // Model matrix: transformations are applied right-to-left order
    model = translation * rotation * scale;

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    glDrawArrays(GL_TRIANGLES, 0, meshes.gTorusMesh.nVertices);

    // Back Light: Neutral, High Intensity
    // 1. Scales the object by 2
    scale = glm::scale(glm::vec3(1.0f, 1.0f, 1.0f));
    // 2. Rotates shape over the x axis
    rotation = glm::rotate(glm::radians(90.0f), glm::vec3(1.0, 0.0f, 0.0f));
    // 3. Place object at the origin
    translation = glm::translate(glm::vec3(3.0f, 7.0f, -5.0f));
    // Model matrix: transformations are applied right-to-left order
    model = translation * rotation * scale;

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    glDrawArrays(GL_TRIANGLES, 0, meshes.gTorusMesh.nVertices);

    glBindVertexArray(0);

    glUseProgram(0);

    // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
    glfwSwapBuffers(gWindow);    // Flips the the back buffer with the front buffer every frame.
}

/*Generate and load the texture*/
bool UCreateTexture(const char* filename, GLuint& textureId)
{
    int width, height, channels;
    unsigned char* image = stbi_load(filename, &width, &height, &channels, 0);
    if (image)
    {
        flipImageVertically(image, width, height, channels);

        glGenTextures(1, &textureId);
        glBindTexture(GL_TEXTURE_2D, textureId);

        // set the texture wrapping parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        // set texture filtering parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        if (channels == 3)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
        else if (channels == 4)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
        else
        {
            cout << "Not implemented to handle image with " << channels << " channels" << endl;
            return false;
        }

        glGenerateMipmap(GL_TEXTURE_2D);

        stbi_image_free(image);
        glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

        return true;
    }

    // Error loading the image
    return false;
}


void UDestroyTexture(GLuint textureId)
{
    glGenTextures(1, &textureId);
}



// Implements the UCreateShaders function
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId)
{
    // Compilation and linkage error reporting
    int success = 0;
    char infoLog[512];

    // Create a Shader program object.
    programId = glCreateProgram();

    // Create the vertex and fragment shader objects
    GLuint vertexShaderId = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);

    // Retrive the shader source
    glShaderSource(vertexShaderId, 1, &vtxShaderSource, NULL);
    glShaderSource(fragmentShaderId, 1, &fragShaderSource, NULL);

    // Compile the vertex shader, and print compilation errors (if any)
    glCompileShader(vertexShaderId); // compile the vertex shader
    // check for shader compile errors
    glGetShaderiv(vertexShaderId, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShaderId, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;

        return false;
    }

    glCompileShader(fragmentShaderId); // compile the fragment shader
    // check for shader compile errors
    glGetShaderiv(fragmentShaderId, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShaderId, sizeof(infoLog), NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;

        return false;
    }

    // Attached compiled shaders to the shader program
    glAttachShader(programId, vertexShaderId);
    glAttachShader(programId, fragmentShaderId);

    glLinkProgram(programId);   // links the shader program
    // check for linking errors
    glGetProgramiv(programId, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(programId, sizeof(infoLog), NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;

        return false;
    }

    glUseProgram(programId);    // Uses the shader program

    return true;
}


void UDestroyShaderProgram(GLuint programId)
{
    glDeleteProgram(programId);
}