// haha, that file is so unusual !
// <Niki> that's because usually we don't declare things left and right, up and down?
// (It doesn't help that some languages plainly forbid circular dependencies)
void minichat_message(char* username, char* message, char *usericonurl, char *userprofileurl);
void minichat_users_at_this_moment(char *string);
void main_start_nicks_update();
void main_end_nicks_update();
void main_add_nick(char *buffer);

