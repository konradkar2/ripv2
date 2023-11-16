#include "test.h"
#include <stdio.h>
#include <string.h>
#include <utils/hashmap.h>

struct user {
	char *name;
	int age;
};

int user_compare(const void *a, const void *b, void *udata)
{
	(void)udata;
	const struct user *ua = a;
	const struct user *ub = b;
	return strcmp(ua->name, ub->name);
}

uint64_t user_hash(const void *item, uint64_t seed0, uint64_t seed1)
{
	const struct user *user = item;
	return hashmap_sip(user->name, strlen(user->name), seed0, seed1);
}

REGISTER_TEST(hashmap)
{
	struct hashmap *map = hashmap_new(sizeof(struct user), 0, 0, 0,
					  user_hash, user_compare, NULL, NULL);

	hashmap_set(map, &(struct user){.name = "Dale", .age = 44});
	hashmap_set(map, &(struct user){.name = "Roger", .age = 68});

    ASSERT(hashmap_count(map) == 2);
    const struct user *user; 

    user = hashmap_get(map, &(struct user){ .name="Jane" });    
    ASSERT(user == NULL);
    user = hashmap_get(map, &(struct user){ .name="Dale" });    
    ASSERT(user && user->age == 44);
    user = hashmap_get(map, &(struct user){ .name="Roger" });    
    ASSERT(user && user->age == 68);

    hashmap_free(map);
}
