// dear imgui: standalone example application for glfw + opengl 3, using programmable pipeline
// (glfw is a cross-platform general purpose library for handling windows, inputs, opengl/vulkan/metal graphics context creation, etc.)

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp
// #define IMGUI_DEFINE_MATH_OPERATORS 
#include "imconfig.h"
#include "vcpkg/vcpkg_installed/x64-osx/include/imgui.h"
#include "vcpkg/vcpkg_installed/x64-osx/include/imgui_impl_glfw.h"
#include "vcpkg/vcpkg_installed/x64-osx/include/imgui_impl_opengl3.h"
#include "vcpkg/vcpkg_installed/x64-osx/include/imgui_internal.h"
#include <algorithm>
#include <cfloat>
#include <climits>
#include <ostream>
#include <vector>
#include <stdio.h>
#define _CRT_SECURE_NO_WARNINGS
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define GL_SILENCE_DEPRECATION
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif
#include "vcpkg/vcpkg_installed/x64-osx/include/GLFW/glfw3.h" // Will drag system OpenGL headers

// custom widgets
// #include "./progressBar.cpp"

// additional headers
#include <filesystem>
#include <string>
#include <sys/stat.h>
#include <iostream>
#include <fstream>

using std::cout;
using std::endl;

//TODO: 
//have minimum size for window 

// [Win32] Our example includes a copy of glfw3.lib pre-compiled with VS2010 to maximize ease of testing and compatibility with old VS compilers.
// To link with VS2010-era libraries, VS2015+ requires linking with legacy_stdio_definitions.lib, which we do using this pragma.
// Your own project should not be affected, as you are likely to link with a newer binary of GLFW that is adequate for your version of Visual Studio.
#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

// This example can also compile and run with Emscripten! See 'Makefile.emscripten' for details.
#ifdef __EMSCRIPTEN__
#include "../libs/emscripten/emscripten_mainloop_stub.h"
#endif

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

// Simple helper function to load an image into a OpenGL texture with common settings
bool LoadTextureFromMemory(const void* data, size_t data_size, GLuint* out_texture, int* out_width, int* out_height)
{
    // Load from file
    int image_width = 0;
    int image_height = 0;
    unsigned char* image_data = stbi_load_from_memory((const unsigned char*)data, (int)data_size, &image_width, &image_height, NULL, 4);
    if (image_data == NULL)
        return false;

    // Create a OpenGL texture identifier
    GLuint image_texture;
    glGenTextures(1, &image_texture);
    glBindTexture(GL_TEXTURE_2D, image_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Upload pixels into texture
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width, image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
    stbi_image_free(image_data);

    *out_texture = image_texture;
    *out_width = image_width;
    *out_height = image_height;

    return true;
}

// Open and read a file, then forward to LoadTextureFromMemory()
bool LoadTextureFromFile(const char* file_name, GLuint* out_texture, int* out_width, int* out_height)
{
    FILE* f = fopen(file_name, "rb");
    if (f == NULL)
        return false;
    fseek(f, 0, SEEK_END);
    size_t file_size = (size_t)ftell(f);
    if (file_size == -1)
        return false;
    fseek(f, 0, SEEK_SET);
    void* file_data = IM_ALLOC(file_size);
    fread(file_data, 1, file_size, f);
    bool ret = LoadTextureFromMemory(file_data, file_size, out_texture, out_width, out_height);
    IM_FREE(file_data);
    return ret;
}

//TODO: check if files are images
//TODO: have this as a bool func?
int LoadImages(const char* path, GLuint* out_textures, int* out_widths, int* out_heights) {
    int ind = 0;
    const std::filesystem::path _path{path};
    for (const auto& entry : std::filesystem::directory_iterator{_path}) {
        std::filesystem::path outfile = entry.path();
        // std::string file_name = outfile.string();
        const char* file_path = outfile.c_str();
        // std::cout << file_path << std::endl;
        bool ret = LoadTextureFromFile(file_path, &out_textures[ind], &out_widths[ind], &out_heights[ind]);
        IM_ASSERT(ret);
        ind++;
    }
    return ind;
}

//Helper functions

//Button center-er 
//take in vector of strings and place the first button at a place where the others would be centered
//spacing for each button -> calcTextSize(label).x + framePadding.x * 2.0f
float GetButtonCenteringOffset(std::vector<char*> labels) {
    ImGuiStyle& style = ImGui::GetStyle();
    float buttonCenteringOffset; 
    for (int i = 0; i < labels.size(); i++) {
        buttonCenteringOffset += ImGui::CalcTextSize(labels[i]).x;
        // buttonCenteringOffset += style.FramePadding.x * 2.0f;
    }
    int count = labels.size();
    buttonCenteringOffset += style.ItemSpacing.x * (count - 1);
    
    float avail = ImGui::GetContentRegionAvail().x;
    
    float off = (avail - buttonCenteringOffset) * 0.5f;
    return off;
}

// Main code
int main(int, char**)
{
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100
    const char* glsl_version = "#version 100";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
    // GL 3.2 + GLSL 150
    const char* glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif

    // Create window with graphics context
    int display_w = 1280, display_h = 720;
    GLFWwindow* window = glfwCreateWindow(display_w, display_h, "Dear ImGui GLFW+OpenGL3 example", nullptr, nullptr);
    std::cout << display_w << " " << display_h << std::endl;
    if (window == nullptr)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext(); 
    ImGuiIO& io = ImGui::GetIO(); (void)io; 
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
#ifdef __EMSCRIPTEN__
    ImGui_ImplGlfw_InstallEmscriptenCanvasResizeCallback("#canvas");
#endif
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    // - Our Emscripten build process allows embedding fonts to be accessible at runtime from the "fonts/" folder. See Makefile.emscripten for details.
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, nullptr, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != nullptr);

    // Our state
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

 //    const std::filesystem::path sandbox{"sandbox"};
 //    std::filesystem::create_directories(sandbox/"dir1"/"dir2");
 //    std::ofstream{sandbox/"file1.txt"};
 //    std::ofstream{sandbox/"file2.txt"};
 // 
 //    std::cout << "directory_iterator:\n";
 //    // directory_iterator can be iterated using a range-for loop
 //    for (auto const& dir_entry : std::filesystem::directory_iterator{sandbox}) 
 //        std::cout << dir_entry.path() << '\n';

    // const std::filesystem::path _path{"/Users/jonathanzhu/Desktop/GestureImages/"};
    // for (const auto& entry : std::filesystem::directory_iterator{_path}) {
    //     std::filesystem::path outfile = entry.path();
    //     std::cout << outfile.c_str() << std::endl;
    // }


    const int MAX = 1e6;
    int CURR_IND = 0;
    int* image_widths = new int[MAX]{};
    int* image_heights = new int[MAX]{};
    GLuint* image_textures= new GLuint[MAX]{};
    //TODO: add support for different os
    int image_count = LoadImages("/Users/jonathanzhu/Desktop/GestureImages/", image_textures, image_widths, image_heights); // remember to use ./ not ../ for paths in same dir
    std::cout << image_count << std::endl;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;

    const std::vector<char*> buttonNames{"Prev", "Pause", "Next"}; //TODO: fix? 


    // Main loop
#ifdef __EMSCRIPTEN__
    // For an Emscripten build we are disabling file-system access, so let's not attempt to do a fopen() of the imgui.ini file.
    // You may manually call LoadIniSettingsFromMemory() to load settings from your own storage.
    io.IniFilename = nullptr;
    EMSCRIPTEN_MAINLOOP_BEGIN
#else
    while (!glfwWindowShouldClose(window))
#endif
    {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        // ImGui::SetNextWindowSize(ImVec2(display_w, display_h));
        // ImFont* font = ImGui::GetDefaultFont();
        // font->FontSize = 0.1f * windowSize.y;

        ImGui::Begin("OpenGL Texture Text");
        
        ImVec2 avail = ImGui::GetContentRegionAvail();
        cout << avail.x << " " << avail.y << endl;
        const float NONIMAGERATIO = 0.1f;
        // std::cout << windowSize.y << " " << windowSize.x << " " << NONIMAGERATIO * windowSize.y << std::endl;
        //Progress bar
        // ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
        ImGui::ProgressBar(0.5);
        // ImGui::PopStyleVar();
        // ImVec2(-FLT_MIN, NONIMAGERATIO * windowSize.y - style.FramePadding.y * 2.0f)
        //Images 
        float widthFactor = image_widths[CURR_IND] / (avail.x * 0.95f);
        float heightFactor = image_heights[CURR_IND] / (avail.y * 0.85f); 
        // std::cout << widthFactor << " " << heightFactor << std::endl;
        float scaleFactor = std::max({widthFactor, heightFactor, 1.0f});
        // cout << scaleFactor << endl; 
        ImVec2 imageSize = ImVec2(image_widths[CURR_IND] / scaleFactor, image_heights[CURR_IND] / scaleFactor);
        
        // std::cout << avail.y << " " << avail.x << " " << imageSize[1] << " " << imageSize[0] << std::endl;
        ImGui::SetCursorPosX((avail.x - imageSize.x) * 0.5f);
        ImGui::Image((void*)(intptr_t)image_textures[CURR_IND], ImVec2(imageSize[0], imageSize[1]));
        // ImGui::NewLine();
        // ImGui::Image((void*)(intptr_t)image_textures[CURR_IND], ImVec2(300, 300));
        
        int i = 0; 
        ImGui::SetCursorPosX(GetButtonCenteringOffset(buttonNames));
        // , ImVec2(0.0f, NONIMAGERATIO * windowSize.y - style.FramePadding.y * 2.0f)
        //Prev button
        if (ImGui::Button(buttonNames[i])) { //TODO: remove if there is no timer (?)
                CURR_IND--; 
        }
        ImGui::SameLine(); i++;

        //Pause button
        if (ImGui::Button(buttonNames[i])) { //TODO: push flag to deactivate when it gets to last image
        }
        ImGui::SameLine(); i++;

        //Next button
        if (ImGui::Button(buttonNames[i])) { //TODO: push flag to deactive when at first image 
                CURR_IND++;
        }
        ImGui::SameLine(); i++;
        // if (CURR_IND < image_count - 1) {
        //     if (ImGui::Button("Next")) {
        //         CURR_IND++;   
        //     }
        //    
        // }
        ImGui::End();
        // // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
        // if (show_demo_window)
        //     ImGui::ShowDemoWindow(&show_demo_window);
        //
        // // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
        // {
        //     static float f = 0.0f;
        //     static int counter = 0;
        //
        //     ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.
        //
        //     ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
        //     ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
        //     ImGui::Checkbox("Another Window", &show_another_window);
        //
        //     ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
        //     ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color
        //
        //     if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
        //         counter++;
        //     ImGui::SameLine();
        //     ImGui::Text("counter = %d", counter);
        //
        //     ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
        //     ImGui::End();
        // }
        //
        // ImGui::Begin("OpenGL Texture Text");
        // ImGui::Text("pointer = %x", my_image_texture);
        // ImGui::Text("size = %d x %d", my_image_width, my_image_height);
        // ImGui::Image((void*)(intptr_t)my_image_texture, ImVec2(my_image_width, my_image_height));
        // ImGui::End();
        //

        // Rendering
        ImGui::Render();
        // int display_w, display_h;
        // glfwGetFramebufferSize(window, &display_w, &display_h);
        // std::cout << display_w << " " << display_h << std::endl;
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }
#ifdef __EMSCRIPTEN__
    EMSCRIPTEN_MAINLOOP_END;
#endif

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
