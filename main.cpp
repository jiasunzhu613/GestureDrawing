// dear imgui: standalone example application for glfw + opengl 3, using programmable pipeline
// (glfw is a cross-platform general purpose library for handling windows, inputs, opengl/vulkan/metal graphics context creation, etc.)

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp
// #define IMGUI_DEFINE_MATH_OPERATORS 
#include "imconfig.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "imgui_internal.h"
#include "base64.h"
// add libpq postgreSQL API to project
#include "libpq-fe.h"
#include "fmt/core.h"
// #include "postgres_ext.h"
// #include "ImGuiFileDialog/ImGuiFileDialog.h"
#include <algorithm>
#include <cfloat>
#include <climits>
#include <cstdlib>
#include <ostream>
#include <vector>
#include <stdio.h>
#define _CRT_SECURE_NO_WARNINGS
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// stored in /usr/local/var/postgresql@15
/*
This formula has created a default database cluster with:
  initdb --locale=C -E UTF-8 /usr/local/var/postgresql@15

postgresql@15 is keg-only, which means it was not symlinked into /usr/local,
because this is an alternate version of another formula.

If you need to have postgresql@15 first in your PATH, run:
  echo 'export PATH="/usr/local/opt/postgresql@15/bin:$PATH"' >> /Users/jonathanzhu/.bash_profile

For compilers to find postgresql@15 you may need to set:
  export LDFLAGS="-L/usr/local/opt/postgresql@15/lib"
  export CPPFLAGS="-I/usr/local/opt/postgresql@15/include"

For pkg-config to find postgresql@15 you may need to set:
  export PKG_CONFIG_PATH="/usr/local/opt/postgresql@15/lib/pkgconfig"

To restart postgresql@15 after an upgrade:
  brew services restart postgresql@15
Or, if you don't want/need a background service you can just run:
  LC_ALL="C" /usr/local/opt/postgresql@15/bin/postgres -D /usr/local/var/postgresql@15
*/
// #include "libpq-fe.h"
#define GL_SILENCE_DEPRECATION
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif
#include <GLFW/glfw3.h> // Will drag system OpenGL headers

// custom widgets
// #include "./progressBar.cpp"

// additional headers
#include <filesystem>
#include <string>
#include <sys/stat.h>
#include <iostream>
#include <fstream>
#include <chrono>
#include <regex>
#include <format>

using std::cout;
using std::endl;

#define USER "/Users/"

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
    cout << image_data << endl;
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

bool LoadTextureFromBase64(std::string base64, GLuint* out_texture, int* out_width, int* out_height)
{
    // decode then forward into Loadtexturefrom memory?
    std::string decoded = base64_decode(base64);
    bool ret = LoadTextureFromMemory(decoded.data(), decoded.size(), out_texture, out_width, out_height);
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

namespace ImGui {
    void Z_Image(ImTextureID user_texture_id, const ImVec2& image_size, ImVec2& uv0, ImVec2& uv1, 
                 const ImVec4& tint_col = ImVec4(1, 1, 1, 1), const ImVec4& border_col = ImVec4(0, 0, 0, 0)) 
    {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems)
            return;
        
        ImGuiIO& IO = ImGui::GetIO();
        ImDrawList* drawlist = ImGui::GetWindowDrawList();
        
        //NOTE: make sure to use GetCursorScreenPos here instead of GetCursorPos, GetCursorScreenPos returns the cursor in window
        ImVec2 cursor_pos = ImGui::GetCursorScreenPos();
        //TODO: add border rect
        drawlist->AddImage((void*)(intptr_t)user_texture_id, cursor_pos, cursor_pos + image_size, ImVec2(uv0[0], uv0[1]), ImVec2(uv1[0], uv1[1]), ImGui::GetColorU32(tint_col));
        ImGui::InvisibleButton("image", image_size);
        
        const bool is_hovered = ImGui::IsItemHovered();
        const bool is_held = ImGui::IsItemActive();

        //UV texture coordinates in decimal form allows it to be used as a zoom ratio to find how much we've zoomed in to the image
        float zoom_ratio = (uv1[0] - uv0[0]) * 1.5f; // 1.5f factor applied just to make the zoom drag speed a little faster 
        //TODO: probably need a scale factor for one dimension of the image

        // Move zoomed image on drag
        if (is_held) {
            float x_dist = uv1[0] - uv0[0];
            float y_dist = uv1[1] - uv0[1];
            ImVec2 mouse_delta = IO.MouseDelta * 0.002f * zoom_ratio;
            // cout << mouse_delta[0] << " " << mouse_delta[1] << endl;
            
            uv0[0] = fmax(0.0f, fmin(1.0f - x_dist, uv0[0] - mouse_delta.x));
            uv0[1] = fmax(0.0f, fmin(1.0f - y_dist, uv0[1] - mouse_delta.y));
            uv1[0] = fmin(1.0f, fmax(0.0f + x_dist, uv1[0] - mouse_delta.x)); 
            uv1[1] = fmin(1.0f, fmax(0.0f + y_dist, uv1[1] - mouse_delta.y)); 
        }
        
        // Zoom in/out on image on scroll
        if (is_hovered) {
            float mouse_wheel_delta = IO.MouseWheel * 0.005f;
            
            // clamp uv0 and uv1 final position so that image does not flip
            float x_clamp = (uv0[0] + uv1[0]) / 2.0f;
            float y_clamp = (uv0[1] + uv1[1]) / 2.0f;
            // uv0 reached a border, so we wait for uv1 to reach a border as well (therefore we only increment uv1)
            if ((uv0[0] == 0.0f || uv0[1] == 0.0f) && (uv1[0] != 1.0f && uv1[1] != 1.0f)) { 
                uv1[0] = fmin(1.0f, fmax(x_clamp, uv1[0] + mouse_wheel_delta));
                uv1[1] = fmin(1.0f, fmax(y_clamp, uv1[1] + mouse_wheel_delta));
            } 
            //uv1 reached a border, so we wait for uv0 to reach a border as well (therefore we only increment uv0)
            else if ((uv1[0] == 1.0f || uv1[1] == 1.0f) && (uv0[0] != 0.0f && uv0[1] != 0.0f)) {
                uv0[0] = fmax(0.0f, fmin(x_clamp, uv0[0] - mouse_wheel_delta));
                uv0[1] = fmax(0.0f, fmin(y_clamp, uv0[1] - mouse_wheel_delta));
            }
            else {
                uv0[0] = fmax(0.0f, fmin(x_clamp, uv0[0] - mouse_wheel_delta));
                uv0[1] = fmax(0.0f, fmin(y_clamp, uv0[1] - mouse_wheel_delta));
                uv1[0] = fmin(1.0f, fmax(x_clamp, uv1[0] + mouse_wheel_delta));
                uv1[1] = fmin(1.0f, fmax(y_clamp, uv1[1] + mouse_wheel_delta));
            }
        }
    }
}

void resetTimer() {
    
}

// PostgreSQL exit nicely. (Why does it need static though?)
static void exit_nicely(PGconn *conn)
{
    PQfinish(conn);
    exit(1);
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
    std::string user = getenv("USER");
    // const std::string USER = "/Users/";
    std::string prefix = USER + user + "/";
    // std::string file_path = prefix;
    // std::string file = "/Users/" + user + "/Desktop/GestureImages"; 
    // cout << file << endl; 
    // int image_count = LoadImages(file.c_str(), image_textures, image_widths, image_heights); // remember to use ./ not ../ for paths in same dir
    int image_count;
    std::cout << image_count << std::endl;

    ImGuiContext& g = *GImGui;
    ImGuiWindow* curr_window = g.CurrentWindow;
    const ImGuiStyle& style = g.Style;
    const std::vector<char*> buttonNames{"Prev", "Pause", "Next"}; //TODO: fix? 

    ImVec2 uv0 = ImVec2(0.0f, 0.0f);
    ImVec2 uv1 = ImVec2(1.0f, 1.0f);

    int paused = 0;
    int was_paused = 0;
    float time_in_minutes = 5.0f;
    double time_limit = time_in_minutes * 60.0f;
    auto start_time{std::chrono::steady_clock::now()};
    auto end_time{std::chrono::steady_clock::now()};
    double prev_elapsed_time_sum{0.0f};

    int session_started = 0;
    
    char file_path[64];
    std::vector<std::filesystem::directory_entry> entries{};

    // Connect to PostgreSQL Db
    const char *connInfo = "host=localhost port=5432 dbname=GestureDrawingDb user=appuser password=123";
    PGconn *conn;
    PGresult *res;
    int nFields;
    // Make connection to DB
    conn = PQconnectdb(connInfo);
    // Check if db was connected successfully
    if (PQstatus(conn) != CONNECTION_OK)
    {
        //print error then exit nicely
        fprintf(stderr, "Connection to database failed: %s",
                PQerrorMessage(conn));
        exit_nicely(conn);
    }

    res = PQexec(conn, "SELECT pg_catalog.set_config('search_path', '', false)");
    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        // print error then exit nicely
        fprintf(stderr, "SET failed: %s", PQerrorMessage(conn));
        PQclear(res);
        exit_nicely(conn);
    }

    /*

     * Clear PGresult whenevr not needed anymore! this helps to prevent memory leaks.
     */
    PQclear(res);

    // Testing Postgresql Features
    int queried = 0;
    char* image_data;
    int image_height;
    int image_width;
    GLuint image_texture;







    
    // ImDrawList* drawlist = ImGui::GetWindowDrawList();

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

        // Testing PGSQL 
        ImGui::SetNextWindowSizeConstraints(ImVec2(200, 200), ImVec2(FLT_MAX, FLT_MAX));
        ImGui::Begin("PGSQL");
        if (ImGui::Button("Add Image"))
        {
            std::string image_path = USER + user + "/desktop/GestureImages/de5f969b107abf8fb70f4828101b063c.jpg";
            // read binary from image file
            FILE* f = fopen(image_path.c_str(), "rb");
            if (f == NULL)
                return false;
            fseek(f, 0, SEEK_END);
            size_t file_size = (size_t)ftell(f);
            if (file_size == -1)
                return false;
            fseek(f, 0, SEEK_SET);
            void* file_data = IM_ALLOC(file_size);
            fread(file_data, 1, file_size, f);
            // cout << (long) file_data << endl;
            // cout << (void*) (long) file_data << endl; // can cast to long then back to void* and retain information
            cout << (const unsigned char*) file_data << endl;
            std::string base64_image_source = base64_encode(reinterpret_cast<unsigned char*>(file_data), file_size);
            //TODO:  add variables for the category and imagebinary field
            std::string formatted = fmt::format("INSERT INTO public.images (category, imageinformation) VALUES ({}, '{}')", "'Figure'", base64_image_source);
            const char *command = formatted.c_str();
            res = PQexec(conn, command);
            IM_FREE(file_data);
            /*
            https://www.postgresql.org/docs/7.0/libpq-chapter22422.htm
            PGRES_COMMAND_OK -- Successful completion of a command returning no data
            PGRES_TUPLES_OK -- The query successfully executed
            */
            if (PQresultStatus(res) != PGRES_COMMAND_OK)
            {
                fprintf(stderr, "INSERT failed: %s", PQerrorMessage(conn)); // Note: fprintf prints into stderr error file but will also output into console!
                PQclear(res);
                exit_nicely(conn);
            }
            PQclear(res);
            cout << "Hello this will add an image!" << endl;
        }
        if (ImGui::Button("Query Images"))
        {
            /*
            Fetch all columns from "Images" db
            */
            
            /*
            query used: select current_database(), current_user, current_setting('search_path')
            result from query in terminal:  GestureDrawingDb | jonathanzhu  | "$user", public

            result from libpq query: current_databasecurrent_user   current_setting  GestureDrawingDbappuser  


            query used: select relname, relnamespace::regnamespace from pg_class where lower(relname) = 'images';
            result from query in terminal:  images  | public
            result from query in libpq: relname        relnamespace   images         public  
            */
            const char *command = "SELECT * FROM public.images WHERE id = 39"; // NOTE: need the public keyword in front for query 
            res = PQexec(conn, command);
            if (PQresultStatus(res) != PGRES_TUPLES_OK)
            {
                // print error then exit nicely
                fprintf(stderr, "SET failed: %s", PQerrorMessage(conn));
                PQclear(res);
                exit_nicely(conn);
            }

            /* first, print out the attribute names */
            // nFields = PQnfields(res);
            // for (int i = 0; i < nFields; i++)
            //     // NOTE: %15s prints the word with a total length of 15, if the word is longer, 
            //     //the whole word will be printed! -15 prints spaces behind word instead of in front
            //     printf("%-15s", PQfname(res, i)); 
            // printf("\n\n");

            // /* next, print out the rows */
            // for (int i = 0; i < PQntuples(res); i++)
            // {
            //     for (int j = 0; j < nFields; j++)
            //         printf("%-15s", PQgetvalue(res, i, j));
            //     printf("\n");
            // }
            
            std::string image_data = PQgetvalue(res, 0, 6);
            cout << image_data << endl;
            bool ret = LoadTextureFromBase64(image_data, &image_texture, &image_width, &image_height);
            IM_ASSERT(ret);
            cout << "hi loaded" << endl;
            // char* _image_data = stbi_zlib_decode_noheader_malloc(image_data.c_str());
            queried = 1;
        }
        if (queried){
            ImGui::Image((void*) (intptr_t) image_texture, ImVec2(image_width, image_height));
        }
        ImGui::End();

        ImGui::SetNextWindowSizeConstraints(ImVec2(200, 200), ImVec2(FLT_MAX, FLT_MAX));
        ImGui::Begin("Gesture Drawing");

        // const float NONIMAGERATIO = 0.1f;
        // std::cout << windowSize.y << " " << windowSize.x << " " << NONIMAGERATIO * windowSize.y << std::endl;
        if (!session_started) {
            //Time limit slider (have buttons for common time limits and link the buttons up with the slider) 
            if (ImGui::Button("1min")) {
                time_in_minutes = 1.0f;
            }
            ImGui::SameLine();
            if (ImGui::Button("3min")) {
                time_in_minutes = 3.0f;
            }
            ImGui::SameLine();
            if (ImGui::Button("5min")) {
                time_in_minutes = 5.0f;
            }
            ImGui::SliderFloat("Time Limit in Minutes", &time_in_minutes, 0.0f, 10.0f, "%.1f");
            //File browsing system
            // ImGui::SetKeyboardFocusHere();
            ImGui::InputText("File Path", file_path, IM_ARRAYSIZE(file_path));
            if(ImGui::IsItemActive()) {
                //NOTE: GetCursorPos returns position of cursor inside current window, so we need to add the current window position by using GetWindowPos
                // ImGui::BringWindowToDisplayFront(ImGui::GetCurrentWindow());
                ImGui::OpenPopup("Autocomplete");
            } else {
                ImGui::CloseCurrentPopup();
            }

            // if (ImGui::IsWindowAppearing())
            //     ImGui::BringWindowToDisplayFront(ImGui::GetCurrentWindow());
            //Set popup position and size
            ImGui::SetNextWindowPos(ImGui::GetCursorPos() + ImGui::GetWindowPos());
            ImVec2 popup_constraint = ImVec2(0.5f * ImGui::GetWindowSize().x, 0.5f * ImGui::GetWindowSize().y);
            ImGui::SetNextWindowSizeConstraints(popup_constraint, popup_constraint);
            if (ImGui::BeginPopup("Autocomplete", ImGuiWindowFlags_NoFocusOnAppearing)) {
                if (std::filesystem::exists(prefix + file_path)) {
                    entries.clear();
                    for (auto const& entry : std::filesystem::directory_iterator{prefix + file_path}) {
                        if (entry.path().c_str()[prefix.length()] != '.')
                            entries.push_back(entry);
                    }
                    std::sort(entries.begin(), entries.end());
                }
                for (auto const& entry : entries){
                    std::filesystem::path outpath = entry.path();
                    std::string suggestion = outpath.string().substr(prefix.length()) + "/"; //TODO: convert to regex eventually

                    std::string temp = prefix + file_path;
                    int ind = temp.find_last_of("/");
                    std::string entered_latest = temp.substr(ind + 1); 
                    std::string regex_rule = entered_latest.empty() ? "[a-z]" : entered_latest; 
                    std::regex rule{regex_rule, std::regex_constants::icase};
                    // cout << regex_rule << endl;
                    if (std::regex_search(suggestion, rule)) {
                        if(ImGui::Selectable(suggestion.c_str())){
                            strcpy(file_path, suggestion.c_str());
                        } 
                        // cout << file_path << endl;
                    }
                }
                ImGui::EndPopup();
            } 

            // NOTE: strcmp returns 0 if strings are equal
            bool disable_start_button = !std::filesystem::exists(prefix + file_path) || strcmp(file_path, "") == 0; //TODO: check if all files are images, maybe also give red text with problem for user 

            ImGui::BeginDisabled(disable_start_button);
            if (ImGui::Button("Start Session")) {
                start_time = std::chrono::steady_clock::now();
                end_time = std::chrono::steady_clock::now();
                image_count = LoadImages((prefix + file_path).c_str(), image_textures, image_widths, image_heights);
                for (int i = 0; i < image_count; i++){
                    cout << image_textures[i] << endl;
                }
                session_started = true;
                memset(file_path, 0, sizeof(file_path));
            }
            ImGui::EndDisabled();
        }
        else {
            // Progress bar
            // ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
            // TODO:: PLEASE FIX THIS TIMER THING, IT LOOKS SO GROSS
            std::chrono::duration<double> current_elapsed_time{end_time - start_time};
            if (current_elapsed_time.count() + prev_elapsed_time_sum > time_limit) {
                start_time = std::chrono::steady_clock::now();
                end_time = std::chrono::steady_clock::now();
                CURR_IND++;
            }
            if (paused && !was_paused){
                prev_elapsed_time_sum += current_elapsed_time.count();
            } 
            if (paused) {
                current_elapsed_time = std::chrono::duration<double>::zero();
                start_time = std::chrono::steady_clock::now();
                end_time = std::chrono::steady_clock::now();
            }
            if (!paused && was_paused){
                start_time = std::chrono::steady_clock::now();
            } 
            if (!paused){
                end_time = std::chrono::steady_clock::now();
            }
            was_paused = paused;
            ImGui::ProgressBar((current_elapsed_time.count() + prev_elapsed_time_sum) / time_limit);
            // ImGui::PopStyleVar();
            // ImVec2(-FLT_MIN, NONIMAGERATIO * windowSize.y - style.FramePadding.y * 2.0f)
            // Quit button
            if (ImGui::Button("Quit")) {
                session_started = false;
            }
            ImGui::SameLine();

            int i = 0; 
            ImGui::SetCursorPosX(GetButtonCenteringOffset(buttonNames));
            // , ImVec2(0.0f, NONIMAGERATIO * windowSize.y - style.FramePadding.y * 2.0f)
            //Prev button
            bool disable_prev = CURR_IND == 0;
            ImGui::BeginDisabled(disable_prev);
            if (ImGui::Button(buttonNames[i])) { //TODO: remove if there is no timer (?)
                paused = 0;
                was_paused = 0;
                prev_elapsed_time_sum = 0.0f;
                start_time = std::chrono::steady_clock::now();
                end_time = std::chrono::steady_clock::now();
                CURR_IND--; 
            }
            ImGui::EndDisabled();
            ImGui::SameLine(); i++;

            //Pause button
            if (ImGui::Button(buttonNames[i])) { //TODO: push flag to deactivate when it gets to last image
                paused ^= 1;
            }
            ImGui::SameLine(); i++;

            //Next button
            bool disable_next = CURR_IND == image_count - 1;
            ImGui::BeginDisabled(disable_next);
            if (ImGui::Button(buttonNames[i])) { //TODO: push flag to deactive when at first image 
                paused = 0;
                was_paused = 0;
                prev_elapsed_time_sum = 0.0f;
                start_time = std::chrono::steady_clock::now();
                end_time = std::chrono::steady_clock::now();
                CURR_IND++;
            }
            ImGui::EndDisabled();
            
            ImVec2 cursor_pos = ImGui::GetCursorPos();
            ImVec2 avail = ImGui::GetContentRegionAvail();
            //Images 
            float widthFactor = image_widths[CURR_IND] / (avail.x);
            float heightFactor = image_heights[CURR_IND] / (avail.y); 
            // std::cout << widthFactor << " " << heightFactor << std::endl;
            float scaleFactor = std::max({widthFactor, heightFactor, 1.0f});
            // cout << scaleFactor << endl; 
            ImVec2 imageSize = ImVec2(image_widths[CURR_IND] / scaleFactor, image_heights[CURR_IND] / scaleFactor);
            
            // std::cout << avail.y << " " << avail.x << " " << imageSize[1] << " " << imageSize[0] << std::endl;
            ImGui::SetCursorPosX((avail.x - imageSize.x) * 0.5f + style.ItemSpacing.x);
            ImGui::SetCursorPosY((avail.y - imageSize.y) * 0.5f + cursor_pos.y);
            ImGui::Z_Image((void*)(intptr_t)image_textures[CURR_IND], ImVec2(imageSize[0], imageSize[1]), uv0, uv1);
            // ImGui::NewLine();
            // ImGui::Image((void*)(intptr_t)image_textures[CURR_IND], ImVec2(300, 300));
            
            // if (CURR_IND < image_count - 1) {
            //     if (ImGui::Button("Next")) {
            //         CURR_IND++;   
            //     }
            //    
            // }
        }
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
