#include <raylib.h>

int main() {
    InitWindow(1280, 720, "JaanBD");

    while(!WindowShouldClose()) {
        BeginDrawing();
            ClearBackground(WHITE);
            DrawText("te", 0, 0, 10, MAROON);
        EndDrawing();
    }
    return 0;
}