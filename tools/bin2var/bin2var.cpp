#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char* argv[])
{
    if (argc < 3) {
        fprintf(stderr, "bin2var /path/to/binary.rom varName\n");
        return -1;
    }
    FILE* fp = fopen(argv[1], "rb");
    if (!fp) {
        fprintf(stderr, "file open error\n");
        return -1;
    }
    fseek(fp, 0, SEEK_END);
    int size = (int)ftell(fp);
    fseek(fp, 0, SEEK_SET);
    printf("const unsigned char rom_%s[%d] = {\n", argv[2], size);
    bool firstLine = true;
    while (1) {
        unsigned char buf[16];
        int readSize = (int)fread(buf, 1, sizeof(buf), fp);
        if (readSize < 1) {
            printf("\n");
            break;
        }
        if (firstLine) {
            firstLine = false;
        } else {
            printf(",\n");
        }
        printf("    ");
        for (int i = 0; i < readSize; i++) {
            if (i) {
                printf(", 0x%02X", buf[i]);
            } else {
                printf("0x%02X", buf[i]);
            }
        }
    }
    printf("};\n");
    fclose(fp);
    return 0;
}
