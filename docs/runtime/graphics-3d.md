# Graphics 3D (OpenGL)

3D rendering via OpenGL. Activated lazily when first 3D function is called.

## Initialization

```nx
import runtime.gfx

fn main() {
    gfx_init(800, 600, "3D Demo")
    defer gfx_quit()
    
    // 3D functions automatically initialize OpenGL context
    let shader = gfx3d_create_shader(vertex_src, fragment_src)
    // ...
}
```

## Shaders

```nx
// Vertex shader
let vertex_src = #"
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;

out vec3 Normal;
out vec2 TexCoord;

void main() {
    Normal = mat3(transpose(inverse(uModel))) * aNormal;
    TexCoord = aTexCoord;
    gl_Position = uProj * uView * uModel * vec4(aPos, 1.0);
}
"#

// Fragment shader
let fragment_src = #"
#version 330 core
in vec3 Normal;
in vec2 TexCoord;

out vec4 FragColor;

uniform vec3 uLightDir;
uniform sampler2D uTexture;

void main() {
    float diff = max(dot(normalize(Normal), -uLightDir), 0.0);
    vec3 color = texture(uTexture, TexCoord).rgb;
    FragColor = vec4(color * diff, 1.0);
}
"#

let shader = gfx3d_create_shader(vertex_src, fragment_src)
```

## Meshes

```nx
// Create from vertices/indices
let vertices = [
    // x, y, z, nx, ny, nz, u, v
    -0.5, -0.5, 0.0,  0,0,1,  0,0,
     0.5, -0.5, 0.0,  0,0,1,  1,0,
     0.0,  0.5, 0.0,  0,0,1,  0.5,1,
]
let indices = [0, 1, 2]

let mesh = gfx3d_create_mesh(vertices, indices)

// Or load from file (OBJ, glTF - future)
let mesh = gfx3d_load_model("assets/model.obj")
```

## Textures

```nx
let tex = gfx3d_load_texture("assets/texture.png")
// Returns texture ID for use in shaders
```

## Rendering

```nx
// Set up matrices
let proj = mat4_perspective(45.0, 800.0/600.0, 0.1, 100.0)
let view = mat4_look_at(vec3(0,0,5), vec3(0,0,0), vec3(0,1,0))
let model = mat4_identity()

// Use shader
gfx3d_use_shader(shader)
gfx3d_set_uniform_mat4(shader, "uProj", proj)
gfx3d_set_uniform_mat4(shader, "uView", view)
gfx3d_set_uniform_mat4(shader, "uModel", model)
gfx3d_set_uniform_vec3(shader, "uLightDir", vec3(0.5, 1.0, 0.3))

// Bind texture
gfx3d_bind_texture(0, tex)

// Draw
gfx3d_draw_mesh(mesh)
```

## Camera

```nx
struct Camera {
    position: vec3,
    target: vec3,
    up: vec3,
    fov: f32,
    aspect: f32,
    near: f32,
    far: f32,
}

fn camera_view(cam: Camera) -> mat4 {
    mat4_look_at(cam.position, cam.target, cam.up)
}

fn camera_proj(cam: Camera) -> mat4 {
    mat4_perspective(cam.fov, cam.aspect, cam.near, cam.far)
}
```

## Math Types

```nx
struct vec3 { x: f32, y: f32, z: f32 }
struct mat4 { m: [f32; 16] }

// Vector ops
vec3_add(a, b), vec3_sub(a, b), vec3_mul(a, b)
vec3_dot(a, b), vec3_cross(a, b), vec3_length(a)
vec3_normalize(a), vec3_lerp(a, b, t)

// Matrix ops
mat4_identity(), mat4_mul(a, b)
mat4_translate(v), mat4_rotate(axis, angle), mat4_scale(v)
mat4_look_at(eye, target, up)
mat4_perspective(fov, aspect, near, far)
mat4_ortho(left, right, bottom, top, near, far)
mat4_inverse(m), mat4_transpose(m)
```

## Complete Example

```nx
import runtime.gfx
import runtime.gfx3d

fn main() {
    gfx_init(800, 600, "3D Triangle")
    defer gfx_quit()
    gfx_vsync(true)

    // Shaders
    let vs = load_shader("shaders/basic.vert")
    let fs = load_shader("shaders/basic.frag")
    let shader = gfx3d_create_shader(vs, fs)

    // Triangle mesh
    let verts = [-0.5, -0.5, 0.0,  1,0,0,  0,0,
                  0.5, -0.5, 0.0,  0,1,0,  1,0,
                  0.0,  0.5, 0.0,  0,0,1,  0.5,1]
    let mesh = gfx3d_create_mesh(verts, [0,1,2])

    // Camera
    let cam = Camera {
        position: vec3(0, 0, 3),
        target: vec3(0, 0, 0),
        up: vec3(0, 1, 0),
        fov: 45.0,
        aspect: 800.0/600.0,
        near: 0.1,
        far: 100.0,
    }

    let mut angle = 0.0

    while !gfx_should_close() {
        let dt = gfx_delta_time()
        angle += dt * 50.0

        gfx_begin_frame()
        gfx_clear(0x1a1a2e)

        // Update
        let model = mat4_rotate(vec3(0,1,0), angle)

        // Render
        gfx3d_use_shader(shader)
        gfx3d_set_uniform_mat4(shader, "uProj", camera_proj(cam))
        gfx3d_set_uniform_mat4(shader, "uView", camera_view(cam))
        gfx3d_set_uniform_mat4(shader, "uModel", model)
        gfx3d_draw_mesh(mesh)

        gfx_end_frame()
    }
}
```