# Renderer

Rex employes a renderer abstraction interface to isolate graphics API code from the actual engine rendering. This is done by `srx/rx/render/frontend`. The documentation of how this frontend interface works is provided here to get you up to speed on how to render things.

## Interface
All rendering resources and commands happen through `frontend::interface`. Every command on the frontend is associated with a tag that tracks the file and line information of the command in the engine as well as a static string describing it, this is provided to the interface with the `RX_RENDER_TAG("string")` macro.

The entire rendering interface is _thread safe_, any and all commands can be called from any thread at any time.

### Resources
There's multiple resource types provided by the frontend, they're listed here.
  * buffer
  * target
  * texture1D
  * texture2D
  * texture3D
  * textureCM
  * program

All resources are created by `interface::create_*()` functions, e.g `interface::create_buffer()` creates a buffer, likewise, all resources are destroyed by `interface::destroy_*()` functions.

Created resources are **not initialized** resources, instead you record information or request requirements of that resource through it's member functions and initialize the resource with `interface::initialize_*()` functions.
Once a resource is initialized it's properties are immutable, **you cannot reinitialize or respecifiy it**.

While resources cannot have their properties reinitialized or respecified, their contents can be updated. This is done with the `interface::update_*()` functions.

Every resource is validated when `interface::initialize_*()` is called. If at any point the resource is not fully specified (something was not recorded or requested), or and attempt was made to record a property or request a requirement that has already been recorded or requested, an assertion will be triggered. These assertions are disabled in release builds.

#### Buffer
A buffer resource represents a combined vertex and element buffer for geometry. The properties that **must be** recorded are provided by the following:
```cpp
// record vertex attributes
// the order of calls is the order of the attributes
// |_type| is the attribute type (e.g float, int)
// |_count| is the # of elements of the attribute type (3 for a vec3)
// |_offset| is the offset in the vertex format this attribute begins in bytes
void record_attribute(attribute::type _type, rx_size _count rx_size _offset);

// record vertex stride
// |_stride| is the size of the vertex format in bytes
void record_stride(rx_size _stride);

// record element type
// |_type| is the element type, k_none for no elements
void record_element_type(element_type _type);

// record type
// |_type| is the buffer type, k_dynamic or k_static
void record_type(type _type);
```

The contents of the buffer can be specified and updated by calling the following functions:
```cpp
// write |_size| bytes from |_data| into vertex store
template<typename T>
void write_vertices(const T* _data, rx_size _size);

// write |_size| bytes from |_data| into element store
template<typename T>
void write_elements(const T* _data, rx_size _size);
```

The functions above _append_ to the vertex and element store, to empty the contents of the buffer call:
```cpp
void flush();
```

Assertions can be triggered in the following cases:
* Not everything was recorded
* The vertex store contains a byte size that is not a multiple of the recorded stride
* The `write_vertices` or `write_elements` functions are called with a size that is not a multiple of `sizeof(T)`.
* The `offset` of an attribute overlaps an existing attribute
* The `offset` of an attribute exceeds the stride of a recorded vertex.
* An attempt was made to modify the buffer after it was initialized.

#### Target
Targets are essentially frame buffer objects. They can contain multiple attachments and they are where the contents of draw call happen on.

Before initializing a target you request certain requirements. Those requirements are requested with the following member functions:

```cpp
// request the swap chain for this target
// you cannot request or attach anything if you request this
void request_swapchain();

// request target have depth attachment |_format| with size |_dimensions|
void request_depth(texture::data_format _format,
  const math::vec2z& _dimensions);

// request target have stencil attachment |_format| with size |_dimensions|
void request_stencil(texture::data_format _format,
  const math::vec2z& _dimensions);

// request target have combined depth stencil attachment with size |_dimensions|
void request_depth_stencil(texture::data_format _format,
  const math::vec2z& _dimensions);
```

A target can also be attached existing textures with the following member functions:

```cpp
// attach existing depth texture |_depth| to target
// can only attach one depth
void attach_depth(texture2D* _depth);

// attach existing stencil texture |_stencil| to target
// can only attach one stencil
void attach_stencil(texture2D* _stencil);

// attach texture |_texture| to target
// the order this function is called is the order the textures are attached
void attach_texture(texture2D* _texture);
```

Assertions can be triggered in the following cases:
* An attempt was made to attach a texture that has already been attached
* An attempt was made to attach more than one depth, stencil or combined depth stencil texture.
* An attempt was made to attach a non-depth and non-stencil texture which has different dimensions than the other attached non-depth and non-stencil textures.
* An attempt was made to attach a non-depth and non-stencil texture which has a different format than the other attached non-depth and non-stencil textures.
* An attempt was made to modify the target after it was initialized.

#### Texture

1D, 2D, 3D and Cubemap textures are used either for drawing to (if attached to a target) or for actual texture mapping during rendering.

Before you can initialize a texture certain information must be recorded by the following member functions:

```cpp
// record format |_format|
void record_format(data_format _format);

// record type
// |_type| can be one of k_static, k_dynamic, k_attachment
// k_static is for textures that will not be updated
// k_dynamic is for textutes that will be updated
// k_attachment is for textures that will not be used in draw calls
void record_type(type _type);

// record filter options |_options|
void record_filter(const filter_options& _options);

// record dimensions |_dimensions|
void record_dimensions(const dimension_type& _dimensions);

// record wrapping behavior |_wrap|
void record_wrap(const wrap_options& _wrap);
```

The contents of the texture can be specified and updated by calling the following functions:

```cpp
// write data |_data| to store for miplevel |_level|
void write(const rx_byte* _data, rx_size _level);

// map data for miplevel |_level|
rx_byte* map(rx_size _level);
```

Assertions can be triggered in the following cases:
* A depth or stencil format was recorded and `k_attachment` was not used for type.
* An attempt was made to write or map a miplevel that is out of bounds.
* An attempt was made to write or map a miplevel other than zero for a texture with no mipmaps.
* An attempt was made to record dimensions that are greater than 4096 pixels in any dimensions.
* An attempt was made to modify the texture after it was initialized.

#### Program

This resource is not actually used directly. It's used to implement `frontend::technique` which provides specialization, variants and program permutations.

It's listed here for completion sake.

Shaders and uniforms are added to a constructed program before initialization with the following member functions:

```cpp
// add a shader definition |_shader|
void add_shader(shader&& _shader);

// add a uniform with name |_name| and type |_type|
uniform& add_uniform(const string& _name, uniform::type _type);
```

Assertions can be triggered in the following cases:
* A shader was added that has already been added (e.g more than one vertex, fragment, compute, etc shader).
* A uniform was added that has already been added.
* An attempt was made to modify the program after it was initialized.

## State
All rendering state aside from bound resources is completely isolated into one single state vector that is passed around for draw calls. This vector is described by `frontend::state` and consists of the following state:

```
scissor_state scissor;
blend_state blend;
depth_state depth;
cull_state cull;
stencil_state stencil;
polygon_state polygon;
```

The state vector is hashed as well as the nested state objects to avoid excessive state changes in the backend. You do not need to manage any state when rendering, state bleed is not possible. This vector can be built every frame for every draw command, the backend is responsible for everything.

## Drawing

Drawing is done with one of two commands:
  * `interface::draw_arrays`
  * `interface::draw_elements`

The `draw_arrays` function is only allowed with buffers that have no elements, e.g `element_kind == k_none`.

Otherwise, the functions behave the exact same way, here's a definition

```cpp
void draw_elements(
  const command_header::info& _info,
  const state& _state,
  target* _target,
  buffer* _buffer,
  program* _program,
  rx_size _count,
  rx_size _offset,
  primitive_type _primitive_type,
  const char* _textures,
  ...);
```

Lets disect what everything is:
* `_info` is the render tag containing file, line and static string information as described at the beginning of this document. You construct one with `RX_RENDER_TAG(...)`.
* `_state` is the entire state vector to use.
* `_target` is the render target to render into.
* `_buffer` is the combined vertex and element buffer to source.
* `_program` is the program to use.
* `_count` is the number of elements to source.
* `_offset` is the offset in the element buffer to begin renering from.
* `_primitive_type` is the primitive type to render, e.g `k_triangles`.
* `_textures` is the texture specification to use (described below)
* `...` is a list of `texture{1D, 2D, 3D, CM}` to use for the draw.

The texture specification string is a string-literal encoding the textures to use for this call using the following table
  * `1`
  * `2`
  * `3`
  * `c`

The string can have a maximum length of `8` (eight), meaning you can have a maximum amount of `8` (eight) textures. There must be exactly as many textures passed in `...` as this string's length. They must mach the specification, e.g passing a `texture1D` when the string says anything other than `1` in that position will trigger an assertion.

Here's an example:

```cpp
draw_elements(
  RX_RENDER_TAG("test"),
  state,
  target,
  buffer,
  program,
  3,
  0,
  primitive_type::k_triangles,
  "123c",
  texture_1d,
  texture_2d,
  texture_3d,
  texture_cm
);
```

## Clearing

Clearing of a render target is done by `interface::clear`, here's the definition:

```cpp
void clear(
  const command_header::info& _info,
  target* _target,
  rx_u32 _clear_mask,
  const math::vec4f& _clear_color
);
```

`_clear_mask` can be one of:
* `RX_RENDER_CLEAR_DEPTH`
* `RX_RENDER_CLEAR_STENCIL`
* `RX_RENDER_CLEAR_COLOR(index)`
* `RX_RENDER_CLEAR_DEPTH | RX_RENDER_CLEAR_STENCIL`

Any other combination of flags is undefined.

`_clear_color` the color for the clear operation, for:
* `RX_RENDER_CLEAR_DEPTH`, `_clear_color.r` stores the depth clear value
* `RX_RENDER_CLEAR_STENCIL`, `_clear_color.r` stores the stencil clear value
* `RX_RENDER_CLEAR_COLOR(index)`, `_clear_color` stores the color clear value
* `RX_RENDER_CLEAR_DEPTH | RX_RENDER_CLEAR_STENCIL`, `_clear_color.r` stores the depth clear, `_clear_color.g` stores the stencil clear


You **cannot** clear depth, color and stencil at the same time. You **cannot** clear multiple color attachments at the same time.

## Queries

You may query information about the renderer with the following member functions:

```cpp
struct statistics {
  rx_size total;
  rx_size used;
  rx_size cached;
  rx_size memory;
};

statistics stats(resource::type _type) const;
rx_size draw_calls() const;
```

The `stats` function in particular can tell you how many objects you can have of that type; `total`, how many are currently in use; `used`, how many are cached; `cached` and how much memory (in bytes) is being used currently for those used objects _last_ frame.

The `draw_calls` function can tell you how many `draw_arrays` and `draw_elements` commands have happened _last_ frame.

In addition, timing information for a frame can be accessed with the following member function:

```cpp
const frame_timer& timer() const &;
```

Which consists of a lot of information that can be queried with the following member functions:

```cpp
rx_f32 mspf() const;
rx_u32 fps() const;
rx_f32 delta_time() const;
rx_f64 resolution() const;
rx_u64 ticks() const;

struct frame_time {
  rx_f64 life;
  rx_f64 frame;
};

const array<frame_time>& frame_times() const &;
```

# Technique