#include "glm/glm.hpp"

class Camera {
    public:
        static inline glm::vec2 cam_offset = { 0.0f, 0.0f };
        static inline glm::vec2 cam_pos = { 0.0f, 0.0f };
        static inline float zoom_factor = 1.0f;
        static inline float cam_ease_factor = 1.0f;

        static void SetPosition(float x, float y);
        static float GetPositionX();
        static float GetPositionY();
        static void SetZoom(float zoom_factor);
        static float GetZoom();
};