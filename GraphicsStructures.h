#pragma once
#include <SDL.h>
#include <algorithm>
#include <iostream>
#include "glm/glm.hpp"

// Estructura para representar un color RGBA
struct Color {
    Uint8 r; // Componente rojo
    Uint8 g; // Componente verde
    Uint8 b; // Componente azul
    Uint8 a; // Componente alfa (transparencia)

    // Constructores para inicializar el color
    Color() : r(0), g(0), b(0), a(255) {}

    Color(int red, int green, int blue, int alpha = 255) {
        // Asegura que los componentes estén en el rango [0, 255]
        r = static_cast<Uint8>(std::min(std::max(red, 0), 255));
        g = static_cast<Uint8>(std::min(std::max(green, 0), 255));
        b = static_cast<Uint8>(std::min(std::max(blue, 0), 255));
        a = static_cast<Uint8>(std::min(std::max(alpha, 0), 255));
    }

    Color(float red, float green, float blue, float alpha = 1.0f) {
        // Convierte valores de punto flotante en enteros en el rango [0, 255]
        r = std::clamp(static_cast<Uint8>(red * 255), Uint8(0), Uint8(255));
        g = std::clamp(static_cast<Uint8>(green * 255), Uint8(0), Uint8(255));
        b = std::clamp(static_cast<Uint8>(blue * 255), Uint8(0), Uint8(255));
        a = std::clamp(static_cast<Uint8>(alpha * 255), Uint8(0), Uint8(255));
    }

    // Sobrecarga del operador + para sumar colores
    Color operator+(const Color& other) const {
        return Color{
                std::min(255, int(r) + int(other.r)),
                std::min(255, int(g) + int(other.g)),
                std::min(255, int(b) + int(other.b)),
                std::min(255, int(a) + int(other.a))
        };
    }

    // Sobrecarga del operador * para escalar colores por un factor
    Color operator*(float factor) const {
        return Color{
                std::clamp(static_cast<Uint8>(r * factor), Uint8(0), Uint8(255)),
                std::clamp(static_cast<Uint8>(g * factor), Uint8(0), Uint8(255)),
                std::clamp(static_cast<Uint8>(b * factor), Uint8(0), Uint8(255)),
                std::clamp(static_cast<Uint8>(a * factor), Uint8(0), Uint8(255))
        };
    }

    friend Color operator*(float factor, const Color& color);
};

// Sobrecarga del operador * para escalar colores por un factor (factor * color)
Color operator*(float factor, const Color& color) {
    return color * factor;
}

// Estructura para representar un vértice con posición y color
struct Vertex {
    glm::vec3 position; // Posición del vértice en el espacio 3D
    Color color; // Color del vértice
};

// Estructura para representar un fragmento con posición y color
struct Fragment {
    glm::vec3 position; // Posición del fragmento en el espacio 3D
    Color color; // Color del fragmento
};

// Estructura para representar uniformes utilizados en gráficos
struct Uniform {
    glm::mat4 model; // Matriz de modelo
    glm::mat4 view; // Matriz de vista
    glm::mat4 projection; // Matriz de proyección
    glm::mat4 viewport; // Matriz de transformación de la vista (viewport)
};
