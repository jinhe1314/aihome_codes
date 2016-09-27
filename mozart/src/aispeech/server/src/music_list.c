#include "music_list.h"

int music_list_alloc(music_obj **obj, int max)
{
	printf("[%s %s %d]\n", __FILE__, __func__, __LINE__);
	*obj = malloc(sizeof(music_obj));
	int retvalue = 0;

	if ((*obj == NULL) || (max == 0)) {
		printf("error:[%s %s %d]\n", __FILE__, __func__, __LINE__);
		retvalue = -1;
		goto error;
	}

	_list_init(&(*obj)->head.list);
	(*obj)->head.title = NULL;
	(*obj)->head.artist = NULL;
	(*obj)->head.url = NULL;
	(*obj)->max = max;
	(*obj)->cur_num = 0;
	(*obj)->cur_music = NULL;
error:	
	return retvalue;
}

music_info *music_cur_get(music_obj *obj)
{
	printf("[%s %s %d]\n", __FILE__, __func__, __LINE__);
	if (obj->cur_music == NULL)
		return NULL;

	music_info *tmp = list_entry(&obj->cur_music->list, music_info, list);
	return (tmp)? tmp: NULL;
}

music_info *music_next_get(music_obj *obj)
{
	printf("[%s %s %d]\n", __FILE__, __func__, __LINE__);
	music_info *next = list_entry(obj->cur_music->list.next,
					music_info,
					list);

	music_info *cur = music_cur_get(obj);
	if (next == cur) {
		return NULL;
	} else {
		obj->cur_music = next;
		return next;
	}
}

music_info *music_prev_get(music_obj *obj)
{
	printf("[%s %s %d]\n", __FILE__, __func__, __LINE__);
	if (obj->cur_music == NULL) {
		return NULL;
	}
	music_info *prev = list_entry(obj->cur_music->list.prev,
					music_info,
					list);
	music_info *cur = music_cur_get(obj);
	if (prev->url == NULL) {
		return NULL;
	}
	
	if (prev == cur) {
		return NULL;
	} else {
		obj->cur_music = prev;
		return prev;
	}
}

int music_list_delete(music_info *info)
{
	printf("[%s %s %d]\n", __FILE__, __func__, __LINE__);
	int retvalue = 0;
	if (info == NULL) {
		printf("error:[%s %s %d]\n", __FILE__, __func__, __LINE__);
		retvalue = -1;
		goto error;
	}
	if (info->url == NULL) {
		printf("[%s %s %d]error: head delete\n", __FILE__, __func__, __LINE__);
	}
	_list_delete(&info->list);
	free(info->title);
	free(info->artist);
	free(info->url);
	free(info);

error:
	return retvalue;
}

int music_list_insert(music_obj *obj, music_info *info)
{
	printf("[%s %s %d]\n", __FILE__, __func__, __LINE__);
	int retvalue = 0;
	if ((info == NULL) || (obj == NULL)) {
		printf("error:[%s %s %d]\n", __FILE__, __func__, __LINE__);
		retvalue = -1;
		goto end;
	}

	if (NULL == info->url) {
		printf("error:[%s %s %d]\n", __FILE__, __func__, __LINE__);
		retvalue = -1;
		goto end;
	}

	if (obj->cur_num > obj->max) {
		printf("error:[%s %s %d]\n", __FILE__, __func__, __LINE__);
		music_info *tmp = list_entry(obj->head.list.next, music_info, list);
		music_list_delete(tmp);
		obj->cur_num--;
	}

	obj->cur_music = info;

	LIST *tmp = &obj->head.list;
	music_info *m;
	while (!is_list_last(tmp)) {
		m = list_entry(tmp, music_info, list);
		if (m->url != NULL) {
			if (0 == strncmp(m->url, info->url, strlen(info->url))) {
				printf("error:[%s %s %d]\n", __FILE__, __func__, __LINE__);
				music_list_delete(m);
				_list_insert_behind(&obj->head.list, &info->list);
				goto end;
			}
		}
		tmp = tmp->next;
	}
	m = list_entry(tmp, music_info, list);
	if (m->url != NULL) {
		if (0 == strncmp(m->url, info->url, strlen(info->url))) {
			printf("error:[%s %s %d]info->url: %s, m->url: %s\n", __FILE__, __func__, __LINE__, info->url, m->url);
			music_list_delete(m);
			obj->cur_num--;
		}
	}
	_list_insert_behind(&obj->head.list, &info->list);
	obj->cur_num++;
end:
	return retvalue;
}

int music_info_alloc(music_info **info, char *title, char *artist, char *url)
{
	printf("[%s %s %d]\n", __FILE__, __func__, __LINE__);
	/*XXX*/
	*info = (music_info *)malloc(sizeof(music_info));
	if (*info == NULL) {
		printf("error:[%s %s %d]\n", __FILE__, __func__, __LINE__);
	}
	if (title != NULL)
		(*info)->title = strdup(title);

	if (artist != NULL)
		(*info)->artist = strdup(artist);

	if (url != NULL)
		(*info)->url = strdup(url);

	return 0;
}

int music_list_destroy(music_obj *obj)
{
	if (obj == NULL) {
		printf("[%s %s %d] error\n", __FILE__, __func__, __LINE__);
		return 0;
	}

	printf("[%s %s %d]\n", __FILE__, __func__, __LINE__);
	LIST *tmp = obj->head.list.next;
	music_info *m;
	while (!is_list_last(tmp)) {
		m = list_entry(tmp, music_info, list);
		tmp = tmp->next;
		music_list_delete(m);
	}
	m = list_entry(tmp, music_info, list);
	if (m == NULL)
		printf("error:[%s %s %d]\n", __FILE__, __func__, __LINE__);

	if (m->url != NULL)
		music_list_delete(m);

	obj->max = 0;
	obj->cur_num = 0;
	obj->cur_music = NULL;
	free(obj);
	obj = NULL;
	return 0;
}
