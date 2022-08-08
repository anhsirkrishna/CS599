
#include "camera.h"

class App
{
public:
    GLFWwindow* GLFW_window;
    App(int argc, char** argv);
    bool doApiDump;
    bool m_show_gui = true;
    int frameCount;
    Camera myCamera;
    void updateCamera();
};
