#include "app.hpp"

#include "gl.hpp"
#include "gl_extensions.hpp"
#include "runtime_config.hpp"

#include <glm/gtx/norm.hpp>
#include <imgui.h>

#include <algorithm>

using namespace gp;

namespace ak_fps_cpp {

    /*
     *
     * https://stackoverflow.com/questions/48979861/numerically-stable-method-for-solving-quadratic-equations/50065711#50065711
     *
      diff_of_products() computes a*b-c*d with a maximum error <= 1.5 ulp

      Claude-Pierre Jeannerod, Nicolas Louvet, and Jean-Michel Muller,
      "Further Analysis of Kahan's Algorithm for the Accurate Computation
      of 2x2 Determinants". Mathematics of Computation, Vol. 82, No. 284,
      Oct. 2013, pp. 2245-2264
    */
    double diff_of_products(double a, double b, double c, double d) {
        double w = d * c;
        double e = fma (-d, c, w);
        double f = fma (a, b, -w);
        return f + e;
    }

    struct QuadraticFormulaResult final {
        bool computeable;
        float x0;
        float x1;
    };

    QuadraticFormulaResult solveQuadratic(float a, float b, float c) {
        QuadraticFormulaResult res;

        float discr = b*b - 4.0f*a*c;

        if (discr < 0) {
            res.computeable = false;
            return res;
        }

        // q = -1/2 * (b +- sqrt(b2 - 4ac))
        float q = -0.5f * (b + copysign(sqrt(discr), b));
        res.computeable = true;
        res.x0 = q/a;
        res.x1 = c/a;
        return res;
    }

    char const g_VertexShader[] = R"(
        #version 330 core

        uniform mat4 uModel;
        uniform mat4 uView;
        uniform mat4 uProjection;

        layout (location = 0) in vec3 aPos;

        void main()
        {
            gl_Position = uProjection * uView * uModel * vec4(aPos, 1.0);
        }
    )";

    char const g_FragmentShader[] = R"(
        #version 330 core

        uniform vec4 uColor;

        out vec4 FragColor;
        out vec3 EntityId;

        void main()
        {
            FragColor = uColor;
        }
    )";

    constexpr std::array<PlainVert, 4> g_CrosshairVerts = {{
       // -X to +X
       {{-0.05f, 0.0f, 0.0f}},
       {{+0.05f, 0.0f, 0.0f}},

       // -Y to +Y
       {{0.0f, -0.05f, 0.0f}},
       {{0.0f, +0.05f, 0.0f}}
    }};

    struct Shader final {
        gl::Program prog = gl::CreateProgramFrom(
            gl::Vertex_shader::from_source(g_VertexShader),
            gl::Fragment_shader::from_source(g_FragmentShader));

        static constexpr gl::Attribute_vec3 aPos{0};
        gl::Uniform_mat4 uModel{prog, "uModel"};
        gl::Uniform_mat4 uView{prog, "uView"};
        gl::Uniform_mat4 uProjection{prog, "uProjection"};
        gl::Uniform_vec4 uColor{prog, "uColor"};
    };

    struct Enemy final {
        glm::vec3 pos;
        bool is_hovered = false;

        Enemy(glm::vec3 pos_) : pos{pos_} {
        }
    };

    // --- BVH stuff ---

    // an extremely basic bump allocator
    template<typename T>
    struct TypedBumpAllocator final {

        // raw bytes for storing T /w type punning
        using T_mem = std::aligned_storage_t<sizeof(T), alignof(T)>;

        // raw byte block containing memory for Ts
        struct Block final {
            size_t nUsed;  // number of Ts in the block
            std::unique_ptr<T_mem[]> data;  // pointer to heap-allocated block head

            // construct a new block by heap-allocating it
            Block(size_t nPerBlock) : nUsed{0}, data{new T_mem[nPerBlock]} {}
        };

        std::vector<Block> blocks;  // allocated blocks
        size_t nPerBlock; // number of Ts that can fit in a newly-allocated block
        int curBlock = -1;  // current block

        // construct allocator - guaranteed not to allocate
        TypedBumpAllocator(size_t nPerBlock_) : nPerBlock{nPerBlock_} {
            GP_ASSERT(nPerBlock > 0);
        }

        // allocate memory for a T, but don't construct the T
        T* alloc() {
            // initial state /w no blocks
            if (curBlock == -1) {
                blocks.emplace_back(nPerBlock);
                curBlock++;
            }

            // block is full, either advance to the next (empty) block
            // or allocate a whole new block
            if (blocks[curBlock].nUsed == nPerBlock) {
                curBlock++;

                if (curBlock >= blocks.size()) {
                    blocks.emplace_back(nPerBlock);
                }
            }

            Block& b = blocks[curBlock];
            T* mem = reinterpret_cast<T*>(b.data.get() + b.nUsed);
            b.nUsed++;
            return mem;
        }

        void reset() {
            for (Block& b : blocks) {
                b.nUsed = 0;
            }
            curBlock = blocks.empty() ? -1 : 0;
        }
    };

    // node of a BVH tree (while building)
    struct BVH_BuildNode final {

        // union of this node with its children/primitives
        AABB bounds;

        // left-hand node, or `nullptr` if this node is a leaf
        BVH_BuildNode* lhs;

        // right-hand node, or `nullptr` if this node is a leaf
        BVH_BuildNode* rhs;

        // offset into primitive info list
        int firstPrimOffset;

        // number of primitives (if leaf)
        int nPrims;
    };

    // info about a primitive
    struct BVH_PrimitiveInfo final {

        // id into the underlying primitives list
        //
        // treat this opaquely - the implementation will use this to
        // index into their own datastructures
        int id;

        // bounds of the primitive
        AABB bounds;
    };

    struct BVH final {
        TypedBumpAllocator<BVH_BuildNode> treemem{128};
        std::vector<BVH_PrimitiveInfo> prims;
        BVH_BuildNode* root = nullptr;

        void reset() {
            treemem.reset();
            prims.resize(0);
            root = nullptr;
        }
    };

    void BVH_InitLeafNode(BVH_BuildNode* n, int firstPrimOffset, int nPrims, AABB bounds) {
        GP_ASSERT(firstPrimOffset >= 0);
        GP_ASSERT(nPrims >= 0);

        n->bounds = bounds;
        n->lhs = nullptr;
        n->rhs = nullptr;
        n->firstPrimOffset = firstPrimOffset;
        n->nPrims = nPrims;
    }

    void BVH_InitInternalNode(BVH_BuildNode* n, BVH_BuildNode* lhs, BVH_BuildNode* rhs) {
        GP_ASSERT(lhs != nullptr);
        GP_ASSERT(rhs != nullptr);

        n->bounds = aabbUnion(lhs->bounds, rhs->bounds);
        n->lhs = lhs;
        n->rhs = rhs;
        n->firstPrimOffset = -1;
        n->nPrims = 0;
    }

    BVH_BuildNode* BVH_RecursiveBuild(BVH& bvh, size_t first, size_t n) {
        std::vector<BVH_PrimitiveInfo>& prims = bvh.prims;

        if (n == 1) {
            // recursion bottomed out: init a leaf

            BVH_BuildNode* rv = bvh.treemem.alloc();
            BVH_InitLeafNode(rv, static_cast<int>(first), 1, prims[first].bounds);
            return rv;
        }

        // else: >1 primitives need to be arranged with internal nodes etc.
        GP_ASSERT(n > 1);

        // compute bounding box of primitive centroids
        AABB centroidAABB{
            {FLT_MAX, FLT_MAX, FLT_MAX},
            {-FLT_MAX, -FLT_MAX, -FLT_MAX}
        };
        for (size_t i = first, last = first + n; i < last; ++i) {
            centroidAABB = aabbUnion(centroidAABB, aabbCenter(prims[i].bounds));  // 50 %
        }

        // edge-case: the bounding box is empty (e.g. because all centroids are
        // at the same location)
        //
        // in this case, return a leaf node
        if (aabbIsEmpty(centroidAABB)) {
            AABB bounds{};
            for (size_t i = first, last = first + n; i < last; ++i) {
                bounds = aabbUnion(bounds, prims[i].bounds);
            }

            BVH_BuildNode* rv = bvh.treemem.alloc();
            BVH_InitLeafNode(rv, first, n, bounds);
            return rv;
        }

        // else: >1 primitives with not-colocated centroids
        GP_ASSERT(n > 1 && !aabbIsEmpty(centroidAABB));

        // as a heuristic, partition along the midpoint of the longest
        // dimension of the centroid AABB

        auto dim = aabbLongestDimension(centroidAABB);
        float pMidx2 = centroidAABB.max[dim] + centroidAABB.min[dim];

        auto isBelowMidpoint = [dim, pMidx2](BVH_PrimitiveInfo const& pi) {
            float pi_pMidx2 = pi.bounds.max[dim] + pi.bounds.min[dim];
            return pi_pMidx2 < pMidx2;
        };

        auto it = std::partition(prims.begin() + first, prims.begin() + first + n, isBelowMidpoint);
        size_t mid = std::distance(prims.begin(), it);

        // we now have two nonempty partitions
        //
        // [first, mid) : everything that has a centroid to the left of the midpoint
        // [mid, n) : everything that has a centroid to the right of the midpoint
        GP_ASSERT(mid != first);
        GP_ASSERT(mid != first + n);
        GP_ASSERT(first < mid && mid < first + n);

        // recurse into the partitions

        BVH_BuildNode* leftNode = BVH_RecursiveBuild(bvh, first, mid-first);
        BVH_BuildNode* rightNode = BVH_RecursiveBuild(bvh, mid, (first + n) - mid);

        BVH_BuildNode* rv = bvh.treemem.alloc();
        BVH_InitInternalNode(rv, leftNode, rightNode);
        return rv;
    }

    void BVH_Build(BVH& bvh, Enemy const* enemies, size_t n) {
        bvh.reset();

        bvh.prims.reserve(n);
        for (size_t i = 0; i < n; ++i) {
            Sphere enemySphere;
            enemySphere.origin = enemies[i].pos;
            enemySphere.radius = 1.0f;
            AABB enemyAABB = sphereAABB(enemySphere);

            BVH_PrimitiveInfo pi;
            pi.id = static_cast<int>(i);
            pi.bounds = enemyAABB;
            bvh.prims.push_back(pi);
        }

        bvh.root = BVH_RecursiveBuild(bvh, 0, bvh.prims.size());
    }

    struct GameScreen final : public Screen {
        Shader shader;

        static gl::Vertex_array makePlainVertVAO(Shader& shader, gl::Array_buffer<PlainVert>& vbo) {
            gl::Vertex_array rv;
            gl::BindVertexArray(rv);
            gl::BindBuffer(vbo);
            gl::VertexAttribPointer(shader.aPos, false, sizeof(PlainVert), offsetof(PlainVert, pos));
            gl::EnableVertexAttribArray(shader.aPos);
            gl::BindVertexArray();
            return rv;
        }

        // cube data
        std::vector<PlainVert> sphereTris = generateUVSphere<PlainVert>();
        AABB cubeAABB = aabbFromVerts(sphereTris);
        Sphere cubeBoundingSphere = boundingSphereFromVerts(sphereTris);
        gl::Array_buffer<PlainVert> cubeVBO{sphereTris};
        gl::Vertex_array cubeVAO = makePlainVertVAO(shader, cubeVBO);

        // crosshair
        gl::Array_buffer<PlainVert> crosshairVBO{g_CrosshairVerts};
        gl::Vertex_array crosshairVAO = makePlainVertVAO(shader, crosshairVBO);

        // wireframe cube (for AABB debugging)
        gl::Array_buffer<PlainVert> cubeWireframeVBO{generateCubeWireMesh()};
        gl::Vertex_array cubeWireframeVAO = makePlainVertVAO(shader, cubeWireframeVBO);

        // quad verts (for drawing planes)
        gl::Array_buffer<PlainVert> quadVBO{generateQuad<PlainVert>()};
        gl::Vertex_array quadVAO = makePlainVertVAO(shader, quadVBO);

        // circle verts (for drawing discs)
        gl::Array_buffer<PlainVert> circleVBO{generateCircle<PlainVert>(36)};
        gl::Vertex_array circleVao = makePlainVertVAO(shader, circleVBO);

        // triangle verts (for testing ray-triangle alg)
        std::array<PlainVert, 3> triangle = {{
            {-10.0f, -10.0f, 0.0f},
            {+0.0f, +10.0f, 0.0f},
            {+10.0f, -10.0f, 0.0f},
        }};
        gl::Array_buffer<PlainVert> triangleVBO{triangle};
        gl::Vertex_array triangleVAO = makePlainVertVAO(shader, triangleVBO);

        // if true, draw AABB wireframes in scene
        bool showAABBs = false;

        // if true, draw BVH wireframes in scene
        bool showBVH = false;

        // BVH (for rendering)
        std::unique_ptr<BVH> bvh = nullptr;

        std::vector<Enemy> enemies = []() {
            constexpr int min = -30;
            constexpr int max = 30;
            constexpr int step = 6;

            std::vector<Enemy> rv;
            for (int x = min; x <= max; x += step) {
                for (int y = min; y <= max; y += step) {
                    for (int z = min; z <= max; z += step) {
                        rv.emplace_back(glm::vec3{x, 50.0f + 2.0f*y, z});
                    }
                }
            }
            return rv;
        }();

        Euler_perspective_camera camera;

        std::chrono::microseconds raycast_dur{0};

        glm::mat4 enemyModelMtx(size_t i) {
            return glm::translate(glm::mat4{1.0f}, enemies[i].pos);
        }

        void onMount() override {
            ImGuiInit();
        }

        void onUnmount() override {
            ImGuiShutdown();
        }

        void onEvent(SDL_Event const& e) override {
            ImGuiOnEvent(e);

            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_q) {
                showAABBs = !showAABBs;
            }

            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_e) {
                showBVH = !showBVH;
            }
        }

        int nqueries = 0;

        void handleCollisionsRecursive(Line const& ray, int& hovered, float& closest, BVH_BuildNode* node) {
            auto [ok1, ignore1, ignore2] = lineIntersectsAABB(node->bounds, ray);
            if (!ok1) {
                return;
            }

            ++nqueries;
            if (node->nPrims > 0) {
                // it's a leaf node
                for (int i = node->firstPrimOffset, end = node->firstPrimOffset + node->nPrims; i < end; ++i) {
                    BVH_PrimitiveInfo const& pi = bvh->prims[i];
                    auto [ok, t0, t1] = lineIntersectsAABB(pi.bounds, ray);
                    if (ok && t0 < closest) {
                        hovered = pi.id;
                        closest = t0;
                    }
                }
            }

            if (node->lhs) {
                // it's an internal node, so recurse
                handleCollisionsRecursive(ray, hovered, closest, node->lhs);
            }

            if (node->rhs) {
                handleCollisionsRecursive(ray, hovered, closest, node->rhs);
            }
        }

        void onUpdate() override {
            camera.onUpdate(10.0f, 0.001f);

            Line ray;
            ray.o = camera.pos;
            ray.d = camera.front();

            int hovered_enemy = -1;
            float closest = FLT_MAX;

            if (!bvh) {
                bvh.reset(new BVH);
            }

            BVH_Build(*bvh, enemies.data(), enemies.size());

            // build a BVH
            auto tbegin = std::chrono::high_resolution_clock::now();

            bool useBvh = !true;
            if (useBvh) {

                // traverse the BVH to find collisions
                nqueries = 0;
                handleCollisionsRecursive(ray, hovered_enemy, closest, bvh->root);

                for (size_t i = 0; i < enemies.size(); ++i) {
                    enemies[i].is_hovered = static_cast<int>(i) == hovered_enemy;
                }
            } else {
                for (size_t i = 0; i < enemies.size(); ++i) {
                    Enemy& e = enemies[i];
                    e.is_hovered = false;  // set true after raytest is done

                    Sphere s;
                    s.origin = e.pos;
                    s.radius = cubeBoundingSphere.radius;

                    AABB aabb = sphereAABB(s);

                    auto [ok, t0, t1] = lineIntersectsAABB(aabb, ray);
                    if (ok && t0 >= 0.0f && t0 < closest) {
                        hovered_enemy = static_cast<int>(i);
                        closest = t0;
                    }
                }
                if (hovered_enemy >= 0) {
                    enemies[static_cast<size_t>(hovered_enemy)].is_hovered = true;
                }
            }

            auto tend = std::chrono::high_resolution_clock::now();
            auto dt = tend - tbegin;
            auto dt_casted = std::chrono::duration_cast<decltype(raycast_dur)>(dt);
            raycast_dur = dt_casted;
        }

        void recurse(BVH_BuildNode* cur, glm::vec4 color) {
            glm::vec3 halfWidths = aabbDimensions(cur->bounds) / 2.0f;
            glm::vec3 center = aabbCenter(cur->bounds);

            glm::mat4 scaler = glm::scale(glm::mat4{1.0f}, halfWidths);
            glm::mat4 mover = glm::translate(glm::mat4{1.0f}, center);
            glm::mat4 modelMtx = mover * scaler;

            gl::Uniform(shader.uColor, color);
            gl::Uniform(shader.uModel, modelMtx);
            gl::DrawArrays(GL_LINES, 0, cubeWireframeVBO.size());

            color.r *= 0.9f;
            if (cur->lhs) {
                recurse(cur->lhs, color);
            }
            if (cur->rhs) {
                recurse(cur->rhs, color);
            }
        }

        void onDraw() override {
            ImGuiNewFrame();

            Line ray;
            ray.o = camera.pos;
            ray.d = camera.front();

            Disc d;
            d.origin = {0.0f, 0.0f, 0.0f};
            d.normal = {0.0f, 1.0f, 0.0f};
            d.radius = {10.0f};

            auto res = lineIntersectsDisc(d, ray);

            ImGui::SetNextWindowSize(ImVec2{200.0f, 200.0f});
            if (ImGui::Begin("frame")) {
                ImGui::Text("FPS = %.2f", ImGui::GetIO().Framerate);
                ImGui::Text("micros = %ld", raycast_dur.count());
                ImGui::Text("nqueries = %i", nqueries);
                ImGui::Text("nels = %zu", enemies.size());
                ImGui::Text("intersects? = %s", res.intersected ? "yes" : "no");
                ImGui::Text("t = %.2f", res.t);
                auto p = camera.pos;
                ImGui::Text("camera %.2f, %.2f, %.f2", p.x, p.y, p.z);
            }
            ImGui::End();

            gl::ClearColor(1.0f, 1.0f, 1.0f, 1.0f);
            gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            gl::UseProgram(shader.prog);

            gl::Uniform(shader.uView, camera.viewMatrix());
            gl::Uniform(shader.uProjection, camera.projectionMatrix(App::cur().aspectRatio()));

            // draw plane
            if (false) {
                Plane p{d.origin, d.normal};
                gl::Uniform(shader.uModel, quadToPlaneXform(p));
                gl::Uniform(shader.uColor, {0.0f, 1.0f, 0.0f, 1.0f});
                gl::BindVertexArray(quadVAO);
                gl::DrawArrays(GL_TRIANGLES, 0, quadVBO.size());
                gl::BindVertexArray();
            }

            // draw disc
            if (true) {
                gl::Uniform(shader.uModel, circleToDiscXform(d));
                auto [ok, t0] = lineIntersectsDisc(d, ray);
                if (ok) {
                    gl::Uniform(shader.uColor, {0.0f, 0.0f, 1.0f, 1.0f});
                } else {
                    gl::Uniform(shader.uColor, {1.0f, 0.0f, 0.0f, 1.0f});
                }

                gl::BindVertexArray(circleVao);
                gl::DrawArrays(GL_TRIANGLES, 0, circleVBO.size());
                gl::BindVertexArray();
            }

            // draw triangle
            if (true) {
                gl::Uniform(shader.uModel, gl::identity_val);

                glm::vec3 tri[3] = {
                    triangle[0].pos,
                    triangle[1].pos,
                    triangle[2].pos,
                };

                auto [ok, t0] = lineIntersectsTriangle(tri, ray);
                if (ok) {
                    gl::Uniform(shader.uColor, {1.0f, 1.0f, 0.0f, 1.0f});
                } else {
                    gl::Uniform(shader.uColor, {0.0f, 1.0f, 1.0f, 1.0f});
                }
                gl::BindVertexArray(triangleVAO);
                gl::DrawArrays(GL_TRIANGLES, 0, triangleVBO.size());
                gl::BindVertexArray();
            }

            gl::BindVertexArray(cubeVAO);
            for (size_t i = 0; i < enemies.size(); ++i) {
                if (enemies[i].is_hovered) {
                    gl::Uniform(shader.uColor, {0.0f, 0.0f, 1.0f, 1.0f});
                } else {
                    gl::Uniform(shader.uColor, {1.0f, 0.0f, 0.0f, 1.0f});
                }

                gl::Uniform(shader.uModel, enemyModelMtx(i));
                gl::DrawArrays(GL_TRIANGLES, 0, cubeVBO.sizei());
            }

            if (showAABBs) {
                gl::BindVertexArray(cubeWireframeVAO);
                gl::Uniform(shader.uColor, {1.0f, 0.0f, 0.0f, 1.0f});

                for (size_t i = 0; i < enemies.size(); ++i) {
                    Enemy const& e = enemies[i];

                    Sphere s;
                    s.origin = e.pos;
                    s.radius = cubeBoundingSphere.radius;

                    AABB aabb = sphereAABB(s);
                    glm::mat4 modelMtx = cubeToAABBXform(aabb);

                    gl::Uniform(shader.uModel, modelMtx);
                    gl::DrawArrays(GL_LINES, 0, cubeWireframeVBO.size());
                }
                gl::BindVertexArray();
            }

            if (showBVH && bvh != nullptr) {
                gl::BindVertexArray(cubeWireframeVAO);
                recurse(this->bvh.get()->root, {1.0f, 0.0f, 0.0f, 1.0f});
                gl::BindVertexArray();
            }

            // draw crosshair
            gl::Uniform(shader.uModel, gl::identity_val);
            gl::Uniform(shader.uView, gl::identity_val);
            gl::Uniform(shader.uProjection, gl::identity_val);
            gl::Uniform(shader.uColor, {0.0f, 0.0f, 0.0f, 0.0f});
            gl::BindVertexArray(crosshairVAO);
            gl::DrawArrays(GL_LINES, 0, crosshairVBO.sizei());

            ImGuiRender();
        }
    };
}

int main(int, char*[]) {
    App app;
    app.enableRelativeMouseMode();
    app.show<ak_fps_cpp::GameScreen>();
}
