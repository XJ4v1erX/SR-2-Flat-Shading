#pragma once
#include "GraphicsStructures.h" // Incluye tus estructuras de datos personalizadas
#include "glm/glm.hpp" // Incluye la biblioteca GLM para operaciones matemáticas
#include <cmath>
#include <random>

// Función para el sombreador de vértices
Vertex vertexShader(const Vertex& vertex, const Uniform& u) {
    // Transforma la posición del vértice usando las matrices proporcionadas en el uniforme
    glm::vec4 v = glm::vec4(vertex.position.x, vertex.position.y, vertex.position.z, 1);
    glm::vec4 r = u.viewport * u.projection * u.view * u.model * v;

    // Devuelve un nuevo vértice transformado
    return Vertex{
            glm::vec3(r.x / r.w, r.y / r.w, r.z / r.w),
            vertex.color
    };
};

// Función para el sombreador de fragmentos (en este caso, simplemente devuelve el fragmento sin cambios)
Fragment fragmentShader(Fragment fragment) {
    return fragment;
};

// Definición de un vector de luz
glm::vec3 light = normalize(glm::vec3(0.5, 0.5, 1));

// Función para calcular las coordenadas baricéntricas de un punto en relación a un triángulo
glm::vec3 barycentricCoordinates(const glm::vec3& P, const glm::vec3& A, const glm::vec3& B, const glm::vec3& C) {
    // Calcula las coordenadas baricéntricas usando la fórmula
    float w = ((B.y - C.y) * (P.x - C.x) + (C.x - B.x) * (P.y - C.y)) /
              ((B.y - C.y) * (A.x - C.x) + (C.x - B.x) * (A.y - C.y));

    float v = ((C.y - A.y) * (P.x - C.x) + (A.x - C.x) * (P.y - C.y)) /
              ((B.y - C.y) * (A.x - C.x) + (C.x - B.x) * (A.y - C.y));

    float u = 1.0f - w - v;

    return {w, v, u};
}

// Función para rasterizar un triángulo en el espacio de pantalla
std::vector<Fragment> triangle(Vertex a, Vertex b, Vertex c) {
    glm::vec3 A = a.position;
    glm::vec3 B = b.position;
    glm::vec3 C = c.position;

    std::vector<Fragment> triangleFragments;

    // Calcula los límites del triángulo en el espacio de pantalla
    float minX = std::min(std::min(A.x, B.x), C.x);
    float minY = std::min(std::min(A.y, B.y), C.y);
    float maxX = std::max(std::max(A.x, B.x), C.x);
    float maxY = std::max(std::max(A.y, B.y), C.y);

    // Convierte los límites a enteros
    minX = std::floor(minX);
    minY = std::floor(minY);
    maxX = std::ceil(maxX);
    maxY = std::ceil(maxY);

    // Calcula la normal del triángulo
    glm::vec3 N = glm::normalize(glm::cross(B - A, C - A));

    // Bucle para rasterizar fragmentos dentro del triángulo
    for (float y = minY; y <= maxY; y++) {
        for (float x = minX; x <= maxX; x++) {
            glm::vec3 P = glm::vec3(x, y, 0);

            glm::vec3 bar = barycentricCoordinates(P, A, B, C);

            // Comprueba si el punto está dentro del triángulo utilizando coordenadas baricéntricas
            if (
                    bar.x <= 1 && bar.x >= 0 &&
                    bar.y <= 1 && bar.y >= 0 &&
                    bar.z <= 1 && bar.z >= 0
                    ) {
                // Calcula la coordenada Z interpolada
                P.z = a.position.z * bar.x + b.position.z * bar.y + c.position.z * bar.z;

                // Calcula la intensidad de la luz y asigna un color al fragmento
                float intensity = glm::dot(N, light) * 10;
                Color color = Color(255 * intensity, 255 * intensity, 255 * intensity);

                // Agrega el fragmento a la lista de fragmentos del triángulo
                triangleFragments.push_back(Fragment{P, color});
            }
        }
    }

    // Devuelve la lista de fragmentos del triángulo
    return triangleFragments;
}
