#include <stdlib.h>
#include <vector>

#include "morpheus.h"
#include "ext/HandmadeMath.h"


char *eat_line(char *stream) {
    while (*stream != '\n' || *stream == '\r') {
        stream++;
    }
    return stream;
}

OBJ_Model load_obj_model(char *file_name) {
    OBJ_Model model{};
    Platform_File file = platform_read_file(file_name);
    char *stream = (char *)file.contents;
    char *end = stream + file.contents_size;
    
    while (stream < end) {
        switch (*stream) {
        case '\n': case '\r':
            stream++;
            break;
        case '#':
            stream = eat_line(stream);
            break;
        case 'o':
            stream = eat_line(stream);
            break;
            
        case 'f': {
            stream++;
            uint32 v0 = 0, v1 = 0, v2 = 0, v3 = 0;
            uint32 vt0 = 0, vt1 = 0, vt2 = 0, vt3 = 0;
            uint32 vn0 = 0, vn1 = 0, vn2 = 0, vn3 = 0;
            
            stream++;
            
            v0 = strtoul(stream, &stream, 10);
            stream++;
            vt0 = strtoul(stream, &stream, 10);
            stream++;
            vn0 = strtoul(stream, &stream, 10);
            stream++;
            
            v1 = strtoul(stream, &stream, 10);
            stream++;
            vt1 = strtoul(stream, &stream, 10);
            stream++;
            vn1 = strtoul(stream, &stream, 10);
            stream++;
            
            v2 = strtoul(stream, &stream, 10);
            stream++;
            vt2 = strtoul(stream, &stream, 10);
            stream++;
            vn2 = strtoul(stream, &stream, 10);

            if (*stream == ' ') {
                stream++;
                v3 = strtoul(stream, &stream, 10);
                stream++;
                vt3 = strtoul(stream, &stream, 10);
                stream++;
                vn3 = strtoul(stream, &stream, 10);
            }

            model.indices.push_back(v0 - 1);
            model.indices.push_back(v1 - 1);
            model.indices.push_back(v2 - 1);
        } break;
        case 'v': {
            stream++;
            if (*stream == ' ') {
                stream++;
                HMM_Vec4 v{};
                v.w = 1.0f;
                v.x = (float)strtod(stream, &stream);
                stream++;
                v.y = (float)strtod(stream, &stream);
                stream++;
                v.z = (float)strtod(stream, &stream);
                if (*stream == ' ') {
                    stream++;
                    v.w = (float)strtod(stream, &stream);
                }
                model.vertices.push_back(v);
            } else if (*stream == 'n') {
                stream++;
                double vn0 = 0, vn1 = 0, vn2 = 0;
                stream++;
                vn0 = strtod(stream, &stream);
                stream++;
                vn1 = strtod(stream, &stream);
                stream++;
                vn2 = strtod(stream, &stream);
            } else if (*stream == 't') {
                stream++;
                double vt0 = 0, vt1 = 0, vt2 = 0;
                stream++;
                vt0 = strtod(stream, &stream);
                if (*stream == ' ') {
                    stream++;
                    vt1 = strtod(stream, &stream);
                    if (*stream == ' ') {
                        stream++;
                        vt2 = strtod(stream, &stream);
                    }
                }

            }
        } break;
        case 's':
            stream = eat_line(stream);
            break;
        default:
            stream++;
        }
    }

    return model;
}
