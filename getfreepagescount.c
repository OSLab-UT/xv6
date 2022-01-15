#include "types.h"
#include "stat.h"
#include "user.h"

int main(void) 
{
    printf(1, "Number of Free Pages : %d\n", get_free_pages_count());
    exit();
} 