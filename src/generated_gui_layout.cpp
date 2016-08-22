void load_layout(GuiContext *ctx)
{
	ctx->layout_count = 0;
	{
		GuiElementLayout l = {0};
		l.id = 209116459;
		GUI_FMT_STR(l.str, sizeof(l.str), "%s", "Select missing paths");
		l.on_same_row = 0;
		l.has_offset = 1;
		l.offset[0] = 55;
		l.offset[1] = 39;
		l.has_size = 1;
		l.size[0] = 237;
		l.size[1] = 378;
		l.prevent_resizing = 0;
		l.align_left = 0;
		l.align_right = 0;
		l.align_top = 0;
		l.align_bottom = 0;
		l.padding[0] = 0;
		l.padding[1] = 0;
		l.padding[2] = 0;
		l.padding[3] = 0;
		l.gap[0] = 0;
		l.gap[1] = 0;
		append_element_layout(ctx, l);
	}

	{
		GuiElementLayout l = {0};
		l.id = 329413595;
		GUI_FMT_STR(l.str, sizeof(l.str), "%s", "gui_bar");
		l.on_same_row = 0;
		l.has_offset = 0;
		l.offset[0] = 0;
		l.offset[1] = 0;
		l.has_size = 1;
		l.size[0] = 0;
		l.size[1] = 25;
		l.prevent_resizing = 0;
		l.align_left = 0;
		l.align_right = 0;
		l.align_top = 0;
		l.align_bottom = 0;
		l.padding[0] = 0;
		l.padding[1] = 0;
		l.padding[2] = 0;
		l.padding[3] = 0;
		l.gap[0] = 0;
		l.gap[1] = 0;
		append_element_layout(ctx, l);
	}

	{
		GuiElementLayout l = {0};
		l.id = 530621610;
		GUI_FMT_STR(l.str, sizeof(l.str), "%s", "camArea");
		l.on_same_row = 0;
		l.has_offset = 1;
		l.offset[0] = 0;
		l.offset[1] = 0;
		l.has_size = 1;
		l.size[0] = 1280;
		l.size[1] = 960;
		l.prevent_resizing = 0;
		l.align_left = 1;
		l.align_right = 1;
		l.align_top = 1;
		l.align_bottom = 1;
		l.padding[0] = 0;
		l.padding[1] = 0;
		l.padding[2] = 0;
		l.padding[3] = 0;
		l.gap[0] = 0;
		l.gap[1] = 0;
		append_element_layout(ctx, l);
	}

	{
		GuiElementLayout l = {0};
		l.id = 869379824;
		GUI_FMT_STR(l.str, sizeof(l.str), "%s", "Animations");
		l.on_same_row = 0;
		l.has_offset = 1;
		l.offset[0] = 735;
		l.offset[1] = 202;
		l.has_size = 1;
		l.size[0] = 205;
		l.size[1] = 735;
		l.prevent_resizing = 0;
		l.align_left = 0;
		l.align_right = 0;
		l.align_top = 0;
		l.align_bottom = 0;
		l.padding[0] = 0;
		l.padding[1] = 0;
		l.padding[2] = 0;
		l.padding[3] = 0;
		l.gap[0] = 0;
		l.gap[1] = 0;
		append_element_layout(ctx, l);
	}

	{
		GuiElementLayout l = {0};
		l.id = 1228705237;
		GUI_FMT_STR(l.str, sizeof(l.str), "%s", "gui_layout_list");
		l.on_same_row = 0;
		l.has_offset = 0;
		l.offset[0] = 0;
		l.offset[1] = 0;
		l.has_size = 1;
		l.size[0] = 100;
		l.size[1] = 20;
		l.prevent_resizing = 0;
		l.align_left = 1;
		l.align_right = 1;
		l.align_top = 0;
		l.align_bottom = 0;
		l.padding[0] = 5;
		l.padding[1] = 0;
		l.padding[2] = 5;
		l.padding[3] = 1;
		l.gap[0] = 0;
		l.gap[1] = 0;
		append_element_layout(ctx, l);
	}

	{
		GuiElementLayout l = {0};
		l.id = 1546540848;
		GUI_FMT_STR(l.str, sizeof(l.str), "%s", "gui_layoutwin");
		l.on_same_row = 0;
		l.has_offset = 1;
		l.offset[0] = 843;
		l.offset[1] = 45;
		l.has_size = 1;
		l.size[0] = 400;
		l.size[1] = 700;
		l.prevent_resizing = 0;
		l.align_left = 0;
		l.align_right = 0;
		l.align_top = 0;
		l.align_bottom = 0;
		l.padding[0] = 0;
		l.padding[1] = 0;
		l.padding[2] = 0;
		l.padding[3] = 0;
		l.gap[0] = 0;
		l.gap[1] = 0;
		append_element_layout(ctx, l);
	}

	{
		GuiElementLayout l = {0};
		l.id = 1614857995;
		GUI_FMT_STR(l.str, sizeof(l.str), "%s", "FMV");
		l.on_same_row = 0;
		l.has_offset = 1;
		l.offset[0] = 392;
		l.offset[1] = 102;
		l.has_size = 1;
		l.size[0] = 617;
		l.size[1] = 506;
		l.prevent_resizing = 0;
		l.align_left = 0;
		l.align_right = 0;
		l.align_top = 0;
		l.align_bottom = 0;
		l.padding[0] = 0;
		l.padding[1] = 0;
		l.padding[2] = 0;
		l.padding[3] = 0;
		l.gap[0] = 0;
		l.gap[1] = 0;
		append_element_layout(ctx, l);
	}

	{
		GuiElementLayout l = {0};
		l.id = 2140526214;
		GUI_FMT_STR(l.str, sizeof(l.str), "%s", "Browsers");
		l.on_same_row = 0;
		l.has_offset = 1;
		l.offset[0] = 25;
		l.offset[1] = 63;
		l.has_size = 1;
		l.size[0] = 206;
		l.size[1] = 250;
		l.prevent_resizing = 0;
		l.align_left = 0;
		l.align_right = 0;
		l.align_top = 0;
		l.align_bottom = 0;
		l.padding[0] = 0;
		l.padding[1] = 0;
		l.padding[2] = 0;
		l.padding[3] = 0;
		l.gap[0] = 0;
		l.gap[1] = 0;
		append_element_layout(ctx, l);
	}

	{
		GuiElementLayout l = {0};
		l.id = 2399145457;
		GUI_FMT_STR(l.str, sizeof(l.str), "%s", "Sound");
		l.on_same_row = 0;
		l.has_offset = 1;
		l.offset[0] = 514;
		l.offset[1] = 205;
		l.has_size = 1;
		l.size[0] = 211;
		l.size[1] = 707;
		l.prevent_resizing = 0;
		l.align_left = 0;
		l.align_right = 0;
		l.align_top = 0;
		l.align_bottom = 0;
		l.padding[0] = 0;
		l.padding[1] = 0;
		l.padding[2] = 0;
		l.padding[3] = 0;
		l.gap[0] = 0;
		l.gap[1] = 0;
		append_element_layout(ctx, l);
	}

	{
		GuiElementLayout l = {0};
		l.id = 2674996296;
		GUI_FMT_STR(l.str, sizeof(l.str), "%s", "Select game");
		l.on_same_row = 0;
		l.has_offset = 1;
		l.offset[0] = 47;
		l.offset[1] = 27;
		l.has_size = 1;
		l.size[0] = 351;
		l.size[1] = 523;
		l.prevent_resizing = 0;
		l.align_left = 0;
		l.align_right = 0;
		l.align_top = 0;
		l.align_bottom = 0;
		l.padding[0] = 0;
		l.padding[1] = 0;
		l.padding[2] = 0;
		l.padding[3] = 0;
		l.gap[0] = 0;
		l.gap[1] = 0;
		append_element_layout(ctx, l);
	}

	{
		GuiElementLayout l = {0};
		l.id = 3012723582;
		GUI_FMT_STR(l.str, sizeof(l.str), "%s", "Audio output settings");
		l.on_same_row = 0;
		l.has_offset = 1;
		l.offset[0] = 25;
		l.offset[1] = 349;
		l.has_size = 1;
		l.size[0] = 324;
		l.size[1] = 215;
		l.prevent_resizing = 0;
		l.align_left = 0;
		l.align_right = 0;
		l.align_top = 0;
		l.align_bottom = 0;
		l.padding[0] = 0;
		l.padding[1] = 0;
		l.padding[2] = 0;
		l.padding[3] = 0;
		l.gap[0] = 0;
		l.gap[1] = 0;
		append_element_layout(ctx, l);
	}

	{
		GuiElementLayout l = {0};
		l.id = 3595896875;
		GUI_FMT_STR(l.str, sizeof(l.str), "%s", "Video player");
		l.on_same_row = 0;
		l.has_offset = 1;
		l.offset[0] = 1040;
		l.offset[1] = 7;
		l.has_size = 1;
		l.size[0] = 212;
		l.size[1] = 795;
		l.prevent_resizing = 0;
		l.align_left = 0;
		l.align_right = 0;
		l.align_top = 0;
		l.align_bottom = 0;
		l.padding[0] = 0;
		l.padding[1] = 0;
		l.padding[2] = 0;
		l.padding[3] = 0;
		l.gap[0] = 0;
		l.gap[1] = 0;
		append_element_layout(ctx, l);
	}

	{
		GuiElementLayout l = {0};
		l.id = 3602310904;
		GUI_FMT_STR(l.str, sizeof(l.str), "%s", "Paths");
		l.on_same_row = 0;
		l.has_offset = 1;
		l.offset[0] = 373;
		l.offset[1] = 212;
		l.has_size = 1;
		l.size[0] = 205;
		l.size[1] = 710;
		l.prevent_resizing = 0;
		l.align_left = 0;
		l.align_right = 0;
		l.align_top = 0;
		l.align_bottom = 0;
		l.padding[0] = 0;
		l.padding[1] = 0;
		l.padding[2] = 0;
		l.padding[3] = 0;
		l.gap[0] = 0;
		l.gap[1] = 0;
		append_element_layout(ctx, l);
	}

	{
		GuiElementLayout l = {0};
		l.id = 3740805772;
		GUI_FMT_STR(l.str, sizeof(l.str), "%s", "gui_client:gui_layoutwin");
		l.on_same_row = 0;
		l.has_offset = 0;
		l.offset[0] = 0;
		l.offset[1] = 0;
		l.has_size = 1;
		l.size[0] = 100;
		l.size[1] = 20;
		l.prevent_resizing = 0;
		l.align_left = 0;
		l.align_right = 0;
		l.align_top = 0;
		l.align_bottom = 0;
		l.padding[0] = 10;
		l.padding[1] = 5;
		l.padding[2] = 10;
		l.padding[3] = 5;
		l.gap[0] = 0;
		l.gap[1] = 4;
		append_element_layout(ctx, l);
	}

	{
		GuiElementLayout l = {0};
		l.id = 3880415376;
		GUI_FMT_STR(l.str, sizeof(l.str), "%s", "gui_slider");
		l.on_same_row = 0;
		l.has_offset = 0;
		l.offset[0] = 0;
		l.offset[1] = 0;
		l.has_size = 1;
		l.size[0] = 15;
		l.size[1] = 15;
		l.prevent_resizing = 0;
		l.align_left = 0;
		l.align_right = 0;
		l.align_top = 0;
		l.align_bottom = 0;
		l.padding[0] = 0;
		l.padding[1] = 0;
		l.padding[2] = 0;
		l.padding[3] = 0;
		l.gap[0] = 0;
		l.gap[1] = 0;
		append_element_layout(ctx, l);
	}

	{
		GuiElementLayout l = {0};
		l.id = 4039296309;
		GUI_FMT_STR(l.str, sizeof(l.str), "%s", "Resource paths");
		l.on_same_row = 0;
		l.has_offset = 1;
		l.offset[0] = 259;
		l.offset[1] = 14;
		l.has_size = 1;
		l.size[0] = 606;
		l.size[1] = 191;
		l.prevent_resizing = 0;
		l.align_left = 0;
		l.align_right = 0;
		l.align_top = 0;
		l.align_bottom = 0;
		l.padding[0] = 0;
		l.padding[1] = 0;
		l.padding[2] = 0;
		l.padding[3] = 0;
		l.gap[0] = 0;
		l.gap[1] = 0;
		append_element_layout(ctx, l);
	}

}
