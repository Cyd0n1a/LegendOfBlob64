/**
 * Ballbox N64 Expansion Pak Demo - Enhanced Physics
 * 
 * Demonstrates:
 * - 8 dynamic rigid bodies (mix of spheres and boxes)
 * - Stable stacking
 * - Hinged box structure
 * - Real-time interaction
 * 
 * Requires: Libdragon with tiny3d, N64 Expansion Pak
 * Memory: ~2-3 MB used for physics, leaving 5-6 MB for rendering/audio
 */

#define BALLBOX_IMPLEMENTATION
#include "ballbox.h"

#include <libdragon.h>
#include <t3d/t3d.h>
#include <t3d/t3dmath.h>
#include <GL/glu.h>

#define FRAME_BUFFER_COUNT 2
#define MAX_BODIES 150
#define MAX_COLLISIONS 500

static display_context_t disp = 0;
static PhysicsWorld* world = NULL;

// Body IDs for easy reference
typedef struct {
    int ground_platform;
    int falling_spheres[4];
    int stacked_boxes[3];
    int hinged_door;
    int door_joint;
} SceneObjects;

static SceneObjects scene;

static void init_graphics(void) {
    display_init(RESOLUTION_320x240, DEPTH_16_BPP, FRAME_BUFFER_COUNT, GAMMA_DEFAULT, ANTIALIAS_RESAMPLE);
    rdpq_init();
    gl_init();
}

static void init_physics_scene(void) {
    // Create world with expansion pak capacity
    world = CreatePhysicsWorld(
        60,    // max_spheres - Can have more now
        60,    // max_boxes
        500    // max_collisions
    );
    
    if (!world) {
        debugf("Failed to create physics world\n");
        return;
    }
    
    // Optimize for N64: fewer iterations but still stable
    SetSolverIterations(world, 8);
    SetDamping(world, 0.96f, 0.96f);
    SetRestitution(world, 0.3f);  // Lower restitution = less bouncing = more stable
    SetManifoldSettings(world, 0.02f, -0.1f, 4);  // 4 contacts per manifold
    
    world->gravity = (Vec3){0, -20, 0};
    world->dt = 1.0f / 30.0f;
    
    // === CREATE GROUND PLATFORM ===
    scene.ground_platform = AddBox(world, 
        (Vec3){0, -8, 0}, 
        (Vec3){15, 1, 15}, 
        QuatIdentity());
    SetBodyStatic(world, scene.ground_platform, true);
    
    // === CREATE FALLING SPHERES (4) ===
    for (int i = 0; i < 4; i++) {
        scene.falling_spheres[i] = AddSphere(world, 
            (Vec3){-8 + i*5, 10 + i*2, -5}, 
            0.8f);
        SetBodyMass(world, scene.falling_spheres[i], 2.0f);
        SetBodyInertiaAuto(world, scene.falling_spheres[i]);
    }
    
    // === CREATE STACKED BOXES (3) ===
    // Box 1 - bottom
    scene.stacked_boxes[0] = AddBox(world,
        (Vec3){10, -5, 0},
        (Vec3){2, 2, 2},
        QuatIdentity());
    SetBodyMass(world, scene.stacked_boxes[0], 5.0f);
    SetBodyInertiaAuto(world, scene.stacked_boxes[0]);
    
    // Box 2 - middle
    scene.stacked_boxes[1] = AddBox(world,
        (Vec3){10, 0, 0},
        (Vec3){2, 2, 2},
        QuatIdentity());
    SetBodyMass(world, scene.stacked_boxes[1], 5.0f);
    SetBodyInertiaAuto(world, scene.stacked_boxes[1]);
    
    // Box 3 - top
    scene.stacked_boxes[2] = AddBox(world,
        (Vec3){10, 5, 0},
        (Vec3){2, 2, 2},
        QuatIdentity());
    SetBodyMass(world, scene.stacked_boxes[2], 5.0f);
    SetBodyInertiaAuto(world, scene.stacked_boxes[2]);
    
    // === CREATE HINGED DOOR STRUCTURE ===
    // Hinge point
    int hinge_anchor = AddBox(world,
        (Vec3){-10, 0, 0},
        (Vec3){0.5f, 0.5f, 0.5f},
        QuatIdentity());
    SetBodyStatic(world, hinge_anchor, true);
    
    // Door (can swing)
    scene.hinged_door = AddBox(world,
        (Vec3){-8, 0, 0},
        (Vec3){1, 3, 0.2f},
        QuatIdentity());
    SetBodyMass(world, scene.hinged_door, 3.0f);
    SetBodyInertiaAuto(world, scene.hinged_door);
    
    // Create hinge joint
    scene.door_joint = AddHingeJoint(world,
        hinge_anchor,
        scene.hinged_door,
        (Vec3){-10, 0, 0},      // world anchor
        (Vec3){0, 0, 1});       // hinge axis (Z)
    
    SetHingeMotor(world, scene.door_joint, false, 0, 0);  // No motor, free swing
    
    debugf("Physics scene initialized\n");
    debugf("Objects: %d bodies, %d spheres, %d boxes\n",
           world->bodies->count, world->spheres->count, world->boxes->count);
}

static void draw_sphere(Vec3 center, float radius) {
    glPushMatrix();
    glTranslatef(center.x, center.y, center.z);
    
    // Very simple sphere: octahedron approximation
    static const float s = 1.0f;
    glBegin(GL_TRIANGLES);
    // Top pyramid
    glVertex3f(s, 0, 0); glVertex3f(0, s, 0); glVertex3f(0, 0, s);
    glVertex3f(0, s, 0); glVertex3f(-s, 0, 0); glVertex3f(0, 0, s);
    glVertex3f(-s, 0, 0); glVertex3f(0, s, 0); glVertex3f(0, 0, -s);
    glVertex3f(0, s, 0); glVertex3f(s, 0, 0); glVertex3f(0, 0, -s);
    // Bottom pyramid
    glVertex3f(s, 0, 0); glVertex3f(0, 0, s); glVertex3f(0, -s, 0);
    glVertex3f(0, 0, s); glVertex3f(-s, 0, 0); glVertex3f(0, -s, 0);
    glVertex3f(-s, 0, 0); glVertex3f(0, 0, -s); glVertex3f(0, -s, 0);
    glVertex3f(0, 0, -s); glVertex3f(s, 0, 0); glVertex3f(0, -s, 0);
    glEnd();
    
    glScalef(radius, radius, radius);
    glPopMatrix();
}

static void draw_box(Vec3 center, Vec3 half_extents, Quat rotation) {
    glPushMatrix();
    glTranslatef(center.x, center.y, center.z);
    
    // Convert quaternion to rotation matrix (simplified)
    // For proper N64 rendering, would use matrix operations
    float w = rotation.w;
    float x = rotation.x;
    float y = rotation.y;
    float z = rotation.z;
    
    float m[16] = {
        1-2*(y*y+z*z), 2*(x*y-w*z), 2*(x*z+w*y), 0,
        2*(x*y+w*z), 1-2*(x*x+z*z), 2*(y*z-w*x), 0,
        2*(x*z-w*y), 2*(y*z+w*x), 1-2*(x*x+y*y), 0,
        0, 0, 0, 1
    };
    glMultMatrixf(m);
    
    float hx = half_extents.x;
    float hy = half_extents.y;
    float hz = half_extents.z;
    
    glBegin(GL_QUADS);
    // Front face
    glVertex3f(-hx, -hy, hz); glVertex3f(hx, -hy, hz); 
    glVertex3f(hx, hy, hz); glVertex3f(-hx, hy, hz);
    // Back face
    glVertex3f(-hx, -hy, -hz); glVertex3f(-hx, hy, -hz); 
    glVertex3f(hx, hy, -hz); glVertex3f(hx, -hy, -hz);
    // Top face
    glVertex3f(-hx, hy, -hz); glVertex3f(-hx, hy, hz); 
    glVertex3f(hx, hy, hz); glVertex3f(hx, hy, -hz);
    // Bottom face
    glVertex3f(-hx, -hy, -hz); glVertex3f(hx, -hy, -hz); 
    glVertex3f(hx, -hy, hz); glVertex3f(-hx, -hy, hz);
    // Right face
    glVertex3f(hx, -hy, -hz); glVertex3f(hx, hy, -hz); 
    glVertex3f(hx, hy, hz); glVertex3f(hx, -hy, hz);
    // Left face
    glVertex3f(-hx, -hy, -hz); glVertex3f(-hx, -hy, hz); 
    glVertex3f(-hx, hy, hz); glVertex3f(-hx, hy, -hz);
    glEnd();
    
    glPopMatrix();
}

static void render_frame(void) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glPerspective(60.0f, 4.0f/3.0f, 0.1f, 100.0f);
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(20, 10, 25,  // camera
              0, 2, 0,     // look at
              0, 1, 0);    // up
    
    // Draw ground platform
    glColor3f(0.5f, 0.5f, 0.5f);
    draw_box(world->boxes->positions[scene.ground_platform],
             world->boxes->sizes[scene.ground_platform],
             world->boxes->rotations[scene.ground_platform]);
    
    // Draw falling spheres
    glColor3f(1.0f, 0.3f, 0.3f);
    for (int i = 0; i < 4; i++) {
        if (scene.falling_spheres[i] >= 0 && 
            scene.falling_spheres[i] < world->spheres->count) {
            draw_sphere(world->spheres->positions[i],
                       world->spheres->radii[i]);
        }
    }
    
    // Draw stacked boxes
    glColor3f(0.3f, 0.7f, 1.0f);
    for (int i = 0; i < 3; i++) {
        int box_idx = scene.stacked_boxes[i];
        if (box_idx >= 0 && box_idx < world->boxes->count) {
            draw_box(world->boxes->positions[box_idx],
                    world->boxes->sizes[box_idx],
                    world->boxes->rotations[box_idx]);
        }
    }
    
    // Draw hinged door
    glColor3f(0.9f, 0.7f, 0.2f);
    if (scene.hinged_door >= 0 && scene.hinged_door < world->boxes->count) {
        draw_box(world->boxes->positions[scene.hinged_door],
                world->boxes->sizes[scene.hinged_door],
                world->boxes->rotations[scene.hinged_door]);
    }
    
    // Debug text overlay
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glOrtho(0, 320, 240, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glDisable(GL_DEPTH_TEST);
    
    // Simple text rendering (would use font system in production)
    debugf("Physics: %d objs | FPS: ~30\n", world->bodies->count);
    debugf("Spheres: %d | Boxes: %d\n", world->spheres->count, world->boxes->count);
    
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glEnable(GL_DEPTH_TEST);
}

static void update_physics(void) {
    PhysicsStep(world, world->dt);
}

static void main_loop(void) {
    while (true) {
        while (!(disp = display_lock())) {}
        
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glClearColor(0.1f, 0.15f, 0.2f, 1.0f);
        
        update_physics();
        render_frame();
        
        display_show(disp);
    }
}

int main(void) {
    console_init();
    console_set_render_mode(RENDER_ACTIVE);
    
    init_graphics();
    init_physics_scene();
    
    debugf("Ballbox N64 Expansion Demo\n");
    debugf("8 Dynamic Bodies\n");
    debugf("Stacking + Hinged Structure\n");
    
    main_loop();
    
    return 0;
}