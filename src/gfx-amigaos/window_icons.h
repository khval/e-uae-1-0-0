
struct kIcon
{
	struct Gadeget *gadget ;
	struct Image *image ;
};

enum
{
	GID_ICONIFY = 1,
	GID_PADLOCK,
	GID_FULLSCREEN,
	GID_PREFS
};

void open_icon( struct Window *win, ULONG imageID, ULONG gadgetID, struct kIcon *icon );
void dispose_icon(struct Window *win, struct kIcon *icon);

