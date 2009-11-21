/****************************************************************************/
/**
 * \file
 * \brief  A gkrellm plugin that displays the exit status of arbitrary shell
 * comands.
 */
/****************************************************************************/


#include <gkrellm2/gkrellm.h>


#define CONFIG_KEYWORD "gkrellexec"
#define PLUGIN_PLACEMENT	MON_MAIL
#define GKRELLEXEC_PROCESSES_MAXNUM 3

#define NMEMB(x) ((sizeof(x))/(sizeof(*x)))


static gchar AboutText[] =
"GKrellM Exec version 0\n\n\n"
"Copyright (C) 2009 by Leandro Penz\n"
"lpenz@terra.com.br\n"
"Released under the GPL\n";


static struct
{
	struct {
		char cmdline[256];
		int timeout;
		struct {
			GtkWidget *cmdline;
			GtkWidget *timeout;
			GkrellmDecal *decaltext;
		}
		widget;
	}
	processes[GKRELLEXEC_PROCESSES_MAXNUM];
	GkrellmPanel *panel;
	gint style_id;
	GkrellmMonitor *plugin_mon;
}
GkrExec;


static void update_plugin(void)
{
	static int first = 1;
	int i;

	if (first) {
		for (i = 0; i < NMEMB(GkrExec.processes); i++)
			gkrellm_draw_decal_text(GkrExec.panel, GkrExec.processes[i].widget.decaltext, "", -1);
		gkrellm_draw_panel_layers(GkrExec.panel);
		first = 0;
		return;
	}

	for (i = 0; i < NMEMB(GkrExec.processes); i++) {
		gkrellm_draw_decal_text(GkrExec.panel, GkrExec.processes[i].widget.decaltext, GkrExec.processes[i].cmdline, -1);
	}

	gkrellm_draw_panel_layers(GkrExec.panel);
}


static void create_plugin(GtkWidget *vbox, gint firstcreate)
{
	GkrellmStyle *style;
	GkrellmMargin *margin;
	GkrellmTextstyle *ts;
	int i;
	gint prevy = 0;
	gint prevh = 0;

	if (firstcreate)
		GkrExec.panel = gkrellm_panel_new0();

	style = gkrellm_meter_style(GkrExec.style_id);
	ts = gkrellm_meter_textstyle(GkrExec.style_id);
	margin = gkrellm_get_style_margins(style);

	for (i = 0; i < NMEMB(GkrExec.processes); i++) {
		GkrExec.processes[i].widget.decaltext = gkrellm_create_decal_text(GkrExec.panel, "WWW", ts, style, 0, prevy + prevh + 2, -1);
		prevy = GkrExec.processes[i].widget.decaltext->y;
		prevh = GkrExec.processes[i].widget.decaltext->h;
		gkrellm_decal_on_top_layer(GkrExec.processes[i].widget.decaltext, TRUE);
		gkrellm_draw_decal_text(GkrExec.panel, GkrExec.processes[i].widget.decaltext, "", -1);
	}

	gkrellm_panel_configure(GkrExec.panel, NULL, style);
	gkrellm_panel_create(vbox, GkrExec.plugin_mon, GkrExec.panel);
	gkrellm_draw_panel_layers(GkrExec.panel);
}


static void create_plugin_tab(GtkWidget * tab_vbox)
{
	GtkWidget *tabs;
	GtkWidget *label;
	GtkWidget *about;
	int i;

	tabs = gtk_notebook_new();
	gtk_notebook_set_tab_pos(GTK_NOTEBOOK(tabs), GTK_POS_TOP);
	gtk_box_pack_start(GTK_BOX(tab_vbox), tabs, TRUE, TRUE, 0);

	for (i = 0; i < NMEMB(GkrExec.processes); i++) {
		GtkWidget *vbox0;
		GtkWidget *hbox;
		char tmp[256];

		/* Tab: */
		snprintf(tmp, sizeof tmp, "Process %d", i + 1);
		vbox0 = gkrellm_gtk_framed_notebook_page(tabs, tmp);

		/* Command line: */
		hbox = gtk_hbox_new(FALSE, 10);
		gtk_box_pack_start(GTK_BOX(vbox0), hbox, FALSE, TRUE, 5);
		label = gtk_label_new("Command line:");
		gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
		GkrExec.processes[i].widget.cmdline = gtk_entry_new_with_max_length(256);
		gtk_box_pack_start(GTK_BOX(hbox), GkrExec.processes[i].widget.cmdline, FALSE, TRUE, 0);
		gtk_entry_set_text(GTK_ENTRY(GkrExec.processes[i].widget.cmdline), GkrExec.processes[i].cmdline);

		/* Timeout: */
		hbox = gtk_hbox_new(FALSE, 10);
		gtk_box_pack_start(GTK_BOX(vbox0), hbox, FALSE, TRUE, 5);
		label = gtk_label_new("Timeout:");
		gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0);
		GkrExec.processes[i].widget.timeout = gtk_entry_new_with_max_length(10);
		gtk_box_pack_start(GTK_BOX(hbox), GkrExec.processes[i].widget.timeout, FALSE, TRUE, 0);
		sprintf(tmp, "%d", GkrExec.processes[i].timeout);
		gtk_entry_set_text(GTK_ENTRY(GkrExec.processes[i].widget.timeout), tmp);
	}

#if 0
	// Info tab  ***
	InfoLabel = gtk_label_new(amiconn_InfoText);
	Label = gtk_label_new("Info");
	gtk_notebook_append_page(GTK_NOTEBOOK(Tabs), InfoLabel, Label);
#endif

	/* About */
	label = gtk_label_new("About");
	about = gtk_label_new(AboutText);
	gtk_notebook_append_page(GTK_NOTEBOOK(tabs), about, label);
}


static void apply_plugin_config(void)
{
	int i;

	for (i = 0; i < NMEMB(GkrExec.processes); i++) {
		snprintf(GkrExec.processes[i].cmdline, sizeof(GkrExec.processes[i].cmdline), "%s", gkrellm_gtk_entry_get_text(&GkrExec.processes[i].widget.cmdline));
		GkrExec.processes[i].timeout = atoi(gkrellm_gtk_entry_get_text(&GkrExec.processes[i].widget.timeout));
	}

#if 0
	/* Re-layout pannel */
	gkrellm_panel_destroy(amiconn_panel);
	CreatePlugin(amiconn_vbox, TRUE);
#endif
}


static void save_plugin_config(FILE *f)
{
	int i;

	for (i = 0; i < NMEMB(GkrExec.processes); i++) {
		if (GkrExec.processes[i].cmdline[0] == 0)
			continue;
		fprintf(f, CONFIG_KEYWORD " cmdline %d %s\n", i, GkrExec.processes[i].cmdline);
		fprintf(f, CONFIG_KEYWORD " timeout %d %d\n", i, GkrExec.processes[i].timeout);
	}
}


static void load_plugin_config(gchar *arg)
{
	gchar config[64], item[256];
	gint n;
	int i;

	n = sscanf(arg, "%s %d %[^\n]", config, &i, item);
	if (n == 3) {
		if (strcmp(config, "cmdline") == 0)
			snprintf(GkrExec.processes[i].cmdline, sizeof(GkrExec.processes[i].cmdline), "%s", item);
		if (strcmp(config, "timeout") == 0)
			sscanf(item, "%d\n", &GkrExec.processes[i].timeout);
	}
}


static GkrellmMonitor plugin_mon =
{
	"gkrellexec",   /* Name, for config tab.        */
	0,              /* Id,  0 if a plugin           */
	create_plugin,  /* The create_plugin() function */
	update_plugin,  /* The update_plugin() function */
	create_plugin_tab,   /* The create_plugin_tab() config function */
	apply_plugin_config, /* The apply_plugin_config() function      */
	save_plugin_config,  /* The save_plugin_config() function  */
	load_plugin_config,  /* The load_plugin_config() function  */
	CONFIG_KEYWORD,      /* config keyword                     */
	NULL,           /* Undefined 2  */
	NULL,           /* Undefined 1  */
	NULL,           /* Undefined 0  */
	PLUGIN_PLACEMENT, /* Insert plugin before this monitor.       */
	NULL,           /* Handle if a plugin, filled in by GKrellM */
	NULL            /* path if a plugin, filled in by GKrellM   */
};


GkrellmMonitor *gkrellm_init_plugin(void)
{
	GkrExec.style_id = gkrellm_add_meter_style(&plugin_mon, CONFIG_KEYWORD);
	GkrExec.plugin_mon = &plugin_mon;
	return &plugin_mon;
}


