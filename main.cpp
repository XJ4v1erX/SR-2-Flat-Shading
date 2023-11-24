#include <SDL.h>
#include <vector>
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include <filesystem>
#include "ShaderUtilities.h"
#include "GraphicsStructures.h"
#include "ObjLoader.h"
#include <array>
#include <fstream>

const int WINDOW_WIDTH = 500;
const int WINDOW_HEIGHT = 500;

// Z-buffer para almacenar información de profundidad de los píxeles
std::array<std::array<float, WINDOW_WIDTH>, WINDOW_HEIGHT> zbuffer;

SDL_Renderer* renderer;

// Estructura uniforme para pasar datos a los shaders
Uniform uniform;

// Función para obtener el directorio actual
std::string getCurrentPath() {
    return std::filesystem::current_path().string();
}

// Función para obtener el directorio padre de una ruta dada
std::string getParentDirectory(const std::string& path) {
    std::filesystem::path filePath(path);
    return filePath.parent_path().string();
}

// Vectores para almacenar vértices y caras del modelo 3D
std::vector<glm::vec3> vertices;
std::vector<Face> faces;

// Colores para borrar y colorear el framebuffer
Color clearColor = {0, 0, 0};
Color currentColor = {255, 255, 255};

// Función para limpiar el framebuffer y el z-buffer
void clear() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);

    // Inicializar el z-buffer con valores máximos
    for (auto &row : zbuffer) {
        std::fill(row.begin(), row.end(), 99999.0f);
    }
}

// Función para dibujar un píxel en el framebuffer y actualizar el z-buffer
void point(Fragment f) {
    if (f.position.y < WINDOW_HEIGHT && f.position.x < WINDOW_WIDTH && f.position.y > 0 && f.position.x > 0 && f.position.z < zbuffer[f.position.y][f.position.x]) {
        SDL_SetRenderDrawColor(renderer, f.color.r, f.color.g, f.color.b, f.color.a);
        SDL_RenderDrawPoint(renderer, f.position.x, f.position.y);
        zbuffer[f.position.y][f.position.x] = f.position.z;
    }
}

// Función para ensamblar los vértices transformados en triángulos
std::vector<std::vector<Vertex>> primitiveAssembly(
        const std::vector<Vertex>& transformedVertices
) {
    std::vector<std::vector<Vertex>> groupedVertices;

    // Agrupar vértices en conjuntos de tres para formar triángulos
    for (int i = 0; i < transformedVertices.size(); i += 3) {
        std::vector<Vertex> vertexGroup;
        vertexGroup.push_back(transformedVertices[i]);
        vertexGroup.push_back(transformedVertices[i+1]);
        vertexGroup.push_back(transformedVertices[i+2]);

        groupedVertices.push_back(vertexGroup);
    }

    return groupedVertices;
}

// Función principal para realizar la renderización
void render(std::vector<glm::vec3> VBO) {
    std::vector<Vertex> transformedVertices;

    // Transformar los vértices del modelo 3D
    for (int i = 0; i < VBO.size(); i++) {
        glm::vec3 v = VBO[i];

        Vertex vertex = {v, Color(255, 255, 255)};
        Vertex transformedVertex = vertexShader(vertex, uniform);
        transformedVertices.push_back(transformedVertex);
    }

    // Ensamblar los triángulos a partir de los vértices transformados
    std::vector<std::vector<Vertex>> triangles = primitiveAssembly(transformedVertices);

    std::vector<Fragment> fragments;
    for (const std::vector<Vertex>& triangleVertices : triangles) {
        // Rasterizar el triángulo y obtener fragmentos
        std::vector<Fragment> rasterizedTriangle = triangle(
                triangleVertices[0],
                triangleVertices[1],
                triangleVertices[2]
        );

        // Agregar los fragmentos al vector de fragmentos
        fragments.insert(
                fragments.end(),
                rasterizedTriangle.begin(),
                rasterizedTriangle.end()
        );
    }

    // Dibujar los fragmentos en el framebuffer
    for (Fragment fragment : fragments) {
        point(fragmentShader(fragment));
    }
}


// Definición de las variables 'a' y 'b' para controlar las transformaciones del modelo
float a = 3.14f / 3.0f;
float b = 0.5f / 3.0f;

// Función para crear la matriz de modelo
glm::mat4 createModelMatrix() {
    // Aumentar el valor de 'b' para rotación
    b += 0.2f;

    // Crear matrices de transformación para la matriz de modelo
    glm::mat4 translation = glm::translate(glm::mat4(1), glm::vec3(-0.05f, -0.09f, 0));
    glm::mat4 rotationY = glm::rotate(glm::mat4(1), glm::radians(a++), glm::vec3(0, 4, 0));
    glm::mat4 rotationX = glm::rotate(glm::mat4(1), glm::radians(b), glm::vec3(1, 0, 0));
    glm::mat4 scale = glm::scale(glm::mat4(1), glm::vec3(0.15f, 0.15f, 0.15f));

    // Combinar las matrices de transformación
    return translation * scale * rotationX * rotationY;
}

// Función para crear la matriz de vista
glm::mat4 createViewMatrix() {

    // Configurar la matriz de vista utilizando glm::lookAt
    // para definir la posición de la cámara, el punto hacia donde mira y la dirección arriba
    return glm::lookAt(
            // donde esta
            glm::vec3(0, 0, -5),
            // hacia adonde mira
            glm::vec3(0, 0, 0),
            // arriba
            glm::vec3(0, 1, 0)
    );
}

// Función para crear la matriz de proyección
glm::mat4 createProjectionMatrix() {
    float fovInDegrees = 20.0f;
    float aspectRatio = WINDOW_WIDTH / WINDOW_HEIGHT;
    float nearClip = 0.1f;
    float farClip = 100.0f;

    return glm::perspective(glm::radians(fovInDegrees), aspectRatio, nearClip, farClip);
}

// Función para crear la matriz de vista en miniatura (viewport)
glm::mat4 createViewportMatrix() {
    glm::mat4 viewport = glm::mat4(1.0f);

    // Escalar
    viewport = glm::scale(viewport, glm::vec3(WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT / 2.0f, 0.5f));

    // Trasladar
    viewport = glm::translate(viewport, glm::vec3(1.0f, 1.0f, 0.5f));

    return viewport;
}

// Función para escribir un archivo BMP a partir del z-buffer
void writeBMP(const std::string& filename) {
    // Obtener el tamaño de la ventana (ancho y alto)
    int width = WINDOW_WIDTH;
    int height = WINDOW_HEIGHT;

    // Encontrar el valor mínimo (zMin) y máximo (zMax) en el z-buffer
    float zMin = std::numeric_limits<float>::max();
    float zMax = std::numeric_limits<float>::lowest();

    for (const auto& row : zbuffer) {
        for (const auto& val : row) {
            if (val != 99999.0f) { // Ignorar valores que no han sido actualizados
                zMin = std::min(zMin, val);
                zMax = std::max(zMax, val);
            }
        }
    }

    // Verificar si zMin y zMax son iguales (posiblemente debido a un error)
    if (zMin == zMax) {
        std::cerr << "zMin y zMax son iguales. Esto producirá una imagen en blanco o negro.\n";
        return;
    }

    // Imprimir los valores de zMin y zMax para referencia
    std::cout << "zMin: " << zMin << ", zMax: " << zMax << "\n";

    // Abrir el archivo BMP en modo binario para escritura
    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "No se pudo abrir el archivo para escribir: " << filename << "\n";
        return;
    }

    // Escribir la cabecera del archivo BMP
    uint32_t fileSize = 54 + 3 * width * height;
    uint32_t dataOffset = 54;
    uint32_t imageSize = 3 * width * height;
    uint32_t biPlanes = 1;
    uint32_t biBitCount = 24;

    uint8_t header[54] = {'B', 'M',
                          static_cast<uint8_t>(fileSize & 0xFF), static_cast<uint8_t>((fileSize >> 8) & 0xFF), static_cast<uint8_t>((fileSize >> 16) & 0xFF), static_cast<uint8_t>((fileSize >> 24) & 0xFF),
                          0, 0, 0, 0,
                          static_cast<uint8_t>(dataOffset & 0xFF), static_cast<uint8_t>((dataOffset >> 8) & 0xFF), static_cast<uint8_t>((dataOffset >> 16) & 0xFF), static_cast<uint8_t>((dataOffset >> 24) & 0xFF),
                          40, 0, 0, 0,
                          static_cast<uint8_t>(width & 0xFF), static_cast<uint8_t>((width >> 8) & 0xFF), static_cast<uint8_t>((width >> 16) & 0xFF), static_cast<uint8_t>((width >> 24) & 0xFF),
                          static_cast<uint8_t>(height & 0xFF), static_cast<uint8_t>((height >> 8) & 0xFF), static_cast<uint8_t>((height >> 16) & 0xFF), static_cast<uint8_t>((height >> 24) & 0xFF),
                          static_cast<uint8_t>(biPlanes & 0xFF), static_cast<uint8_t>((biPlanes >> 8) & 0xFF),
                          static_cast<uint8_t>(biBitCount & 0xFF), static_cast<uint8_t>((biBitCount >> 8) & 0xFF)};

    file.write(reinterpret_cast<char*>(header), sizeof(header));

    // Escribir los datos de los píxeles en el archivo BMP
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            // Normalizar los valores de profundidad en el z-buffer
            float normalized = (zbuffer[y][x] - zMin) / (zMax - zMin);

            // Convertir el valor normalizado en un color de píxel y escribirlo en el archivo
            auto color = static_cast<uint8_t>(normalized * 255);
            file.write(reinterpret_cast<char*>(&color), 1);
            file.write(reinterpret_cast<char*>(&color), 1);
            file.write(reinterpret_cast<char*>(&color), 1);
        }
    }

    // Cerrar el archivo BMP
    file.close();
    std::cout << "Archivo BMP guardado: " << filename << "\n";
}




int main(int argc, char** argv) {
    SDL_Init(SDL_INIT_EVERYTHING);

    // Crear una ventana SDL
    SDL_Window* window = SDL_CreateWindow("Spaceship", 100, 100, WINDOW_WIDTH, WINDOW_HEIGHT, 0);

    // Obtener el directorio actual y cargar un archivo OBJ
    std::string currentPath = getCurrentPath();
    std::string fileName = "spaceship.obj";
    std::string filePath = getParentDirectory(currentPath) + "\\" + fileName;
    loadOBJ(filePath, vertices, faces);

    // Ajustar la orientación del modelo 3D
    glm::vec3 rotationAngles = glm::vec3(125, 120, 50); // Ajusta estos ángulos para la orientación deseada
    for (auto& vertex : vertices) {
        vertex = rotateVertex(vertex, rotationAngles);
    }

    // Crear un arreglo de vértices del modelo 3D
    std::vector<glm::vec3> vertexArray = setupVertexArray(vertices, faces);

    // Crear el renderizador SDL
    renderer = SDL_CreateRenderer(
            window,
            -1,
            SDL_RENDERER_ACCELERATED
    );

    bool running = true;
    SDL_Event event;

    // Arreglo de vértices para un objeto 3D simple
    std::vector<glm::vec3> vertexBufferObject = {
            {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f},
            {-0.87f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f},
            {0.87f,  -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f},

            {0.0f, 1.0f,    -1.0f}, {1.0f, 1.0f, 0.0f},
            {-0.87f, -0.5f, -1.0f}, {0.0f, 1.0f, 1.0f},
            {0.87f,  -0.5f, -1.0f}, {1.0f, 0.0f, 1.0f}
    };

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
            if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    // Manejar eventos de teclado aquí
                }
            }
        }

        // Configurar las matrices de transformación
        uniform.model = createModelMatrix();
        uniform.view = createViewMatrix();
        uniform.projection = createProjectionMatrix();
        uniform.viewport = createViewportMatrix();

        // Limpiar el framebuffer
        clear();

        // Realizar la renderización
        render(vertexArray);

        // Presentar el framebuffer en la ventana
        SDL_RenderPresent(renderer);

        // Retardo para limitar la velocidad de cuadros
        SDL_Delay(1000 / 144);

        // Guardar el z-buffer en un archivo BMP
        writeBMP("../Spaceship.bmp");

    }

    // Limpiar y cerrar SDL
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}