#include <shapefil.h>
#include <cstdio>
#include <map>

enum mode_e {
    MODE_ELEVATION_FROM_MEASURE = 0,
    MODE_ELEVATION_ZERO = 1,
};

struct src {
    char const * filename;
    mode_e mode;
};

int main(void) {

    src sources[] = {
        { "data/nz-mainland-contours-topo/nz-mainland-contours-topo-1", MODE_ELEVATION_FROM_MEASURE },
        { "data/nz-mainland-contours-topo/nz-mainland-contours-topo-2", MODE_ELEVATION_FROM_MEASURE },
        { "data/nz-mainland-contours-topo/nz-mainland-contours-topo-3", MODE_ELEVATION_FROM_MEASURE },
        { "data/nz-mainland-contours-topo/nz-mainland-contours-topo-4", MODE_ELEVATION_FROM_MEASURE },
        { "data/nz-coastlines-and-islands/nz-coastlines-and-islands", MODE_ELEVATION_ZERO },
        { 0, MODE_ELEVATION_ZERO },
    };

    for (src *s = sources; s->filename; s++) {
        printf("considering %s\n", s->filename);
        SHPHandle sh = SHPOpen(s->filename, "rb");

        int num_entities;
        SHPGetInfo(sh, &num_entities, 0, 0, 0);
        printf("entities: %d\n", num_entities);

        for (int i = 0; i < num_entities; i++) {
            SHPObject * o = SHPReadObject(sh, i);
            SHPDestroyObject(o);
        }

        SHPClose(sh);
    }
}
