#include "yocto/yocto_gl.h"
#include "yocto/yocto_gl.cpp"
#include "voro++-0.4.6/src/voro++.hh"

using namespace ygl;

// Genera in modo randomico i punti di attrazione
voro::container throw_darts(int N, vec2f p0, vec2f p1, vec2f t0, vec2f t1)
{
    auto points = std::vector<vec3f>();
    auto bbox = bbox3f();
    const auto eps = 0.02f;
    auto rng = init_rng(time(nullptr));

    for (auto i = 0; i < N; i++)
    {
        auto y = next_rand1f(rng, p0.x, p1.x);
        auto p = eval_bezier_cubic(p1, t1, t0, p0, y);
        auto xz = next_rand2f(rng, -p, p);

        points.push_back({xz.x, y, xz.y});

        bbox += {xz.x, y, xz.y};
    }

    auto vorodiag = voro::container(bbox.min.x - eps, bbox.max.x + eps, bbox.min.y - eps, bbox.max.y + eps,
                                    bbox.min.z - eps, bbox.max.z + eps, N / 5, N / 5, N / 5, false, false, false, 8);

    for (auto i = 0; i < N; i++)
        vorodiag.put(i, points[i].x, points[i].y, points[i].z);

    return vorodiag;
}

void grow();

int main()
{
    auto N = 12000;
    auto iter_num = 10;
    auto D = 0.1f;
    auto dk = D * 2;
    auto di = D * 17;
    auto p0 = vec2f{0, 0};
    auto p1 = vec2f{1, 0};
    auto t0 = vec2f{0.6, 0.2};
    auto t1 = vec2f{0.1, 0.4};
    auto nodes_id = 0;
    auto node_id = 0;
    auto attr_id = 0;
    auto search_id = 0;

    auto reder = 0.0;

    auto voro_attr = throw_darts(N, p0, p1, t0, t1);
    auto attr_loop = voro::c_loop_subset(voro_attr);

    auto voro_nodes = voro::container(voro_attr.ax, voro_attr.bx, min(voro_attr.ay, -0.02f), voro_attr.by,
                                      voro_attr.az, voro_attr.bz, N / 5, N / 5, N / 5, false, false, false, 8);
    voro_nodes.put(nodes_id++, 0.0f, 0.0f, 0.0f);
    auto nodes_loop = voro::c_loop_all(voro_nodes);

    auto dead_attr = std::unordered_set<int>();

    auto x = 0.0, y = 0.0, z = 0.0;
    for (auto i = 0; i < iter_num; i++)
    {
        if (nodes_loop.start())
            do
            {
                nodes_loop.pos(node_id, x, y, z, reder);
                auto node = vec3f {(float) x, (float) y, (float) z};

                attr_loop.setup_sphere(node.x, node.y, node.z, di, true);

                auto sum = vec3f {0, 0, 0};

                if (attr_loop.start())
                    do
                    {
                        attr_loop.pos(attr_id, x, y, z, reder);

                        if (dead_attr.count(attr_id))
                            continue;

                        voro_nodes.find_voronoi_cell(x, y, z, x, y, z, search_id);

                        if (search_id != node_id)
                            continue;

                        auto attr = vec3f {(float) x, (float) y, (float) z};
                        sum += normalize(attr - node);
                    } while (attr_loop.inc());

                auto new_node = node + D * sum;
                voro_nodes.put(node_id++, new_node.x, new_node.y, new_node.z);

                attr_loop.setup_sphere(new_node.x, new_node.y, new_node.z, dk, true);
                if (attr_loop.start())
                    do
                    {
                        attr_loop.pos(attr_id, x, y, z, reder);
                        dead_attr.insert(attr_id);
                    } while (attr_loop.inc());

            } while (nodes_loop.inc());
    }

    nodes_loop.start();
    do
    {
        nodes_loop.pos(node_id, x, y, z, reder);
        printf("Nodo %d := \t x = %f , y = %f , z = %f\n", node_id, x, y, z);
    } while (nodes_loop.inc());

    return 0;
}