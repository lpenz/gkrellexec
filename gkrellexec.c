/****************************************************************************/
/**
 * \file
 * \brief  A gkrellm plugin that displays the exit status of arbitrary shell
 * comands.
 */
/****************************************************************************/


#include <gkrellm2/gkrellm.h>


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
		GtkWidget *cmdlineWidget;
		GtkWidget *timeoutWidget;
		char cmdline[256];
		int timeout;
	}
	processes[1];
}
Config;


static void update_plugin()
{
	/* See examples below */
}


static void create_plugin(GtkWidget *vbox, gint first_create)
{
	/* See examples below */
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

	for(i = 0; i < NMEMB(Config.processes); i++) {
		GtkWidget *vbox0;
		GtkWidget *hbox;
		char tmp[256];

		/* Tab: */
		snprintf(tmp, sizeof tmp, "Process %d", i+1);
		vbox0 = gkrellm_gtk_framed_notebook_page(tabs, tmp);

		/* Command line: */
		hbox = gtk_hbox_new(FALSE, 10);
		gtk_box_pack_start(GTK_BOX(vbox0), hbox, FALSE, TRUE, 5);
		label = gtk_label_new("Command line:");
		gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
		Config.processes[i].cmdlineWidget = gtk_entry_new_with_max_length(256);
		gtk_box_pack_start(GTK_BOX(hbox), Config.processes[i].cmdlineWidget, FALSE, TRUE, 0);
		gtk_entry_set_text(GTK_ENTRY(Config.processes[i].timeoutWidget), Config.processes[i].cmdline);

		/* Timeout: */
		hbox = gtk_hbox_new(FALSE, 10);
		gtk_box_pack_start(GTK_BOX(vbox0), hbox, FALSE, TRUE, 5);
		label = gtk_label_new("Timeout:");
		gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0);
		Config.processes[i].timeoutWidget = gtk_entry_new_with_max_length(10);
		gtk_box_pack_start(GTK_BOX(hbox), Config.processes[i].timeoutWidget, FALSE, TRUE, 0);
		sprintf(tmp, "%d", Config.processes[i].timeout);
		gtk_entry_set_text(GTK_ENTRY(Config.processes[i].timeoutWidget), tmp);
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


static GkrellmMonitor  plugin_mon  =
{
	"gkrellexec",   /* Name, for config tab.        */
	0,              /* Id,  0 if a plugin           */
	create_plugin,  /* The create_plugin() function */
	update_plugin,  /* The update_plugin() function */
	create_plugin_tab, /* The create_plugin_tab() config function */
	NULL,           /* The apply_plugin_config() function      */
	NULL,           /* The save_plugin_config() function  */
	NULL,           /* The load_plugin_config() function  */
	"gkrellexec",   /* config keyword                     */
	NULL,           /* Undefined 2  */
	NULL,           /* Undefined 1  */
	NULL,           /* Undefined 0  */
	PLUGIN_PLACEMENT, /* Insert plugin before this monitor.       */
	NULL,           /* Handle if a plugin, filled in by GKrellM */
	NULL            /* path if a plugin, filled in by GKrellM   */
}
;


GkrellmMonitor *gkrellm_init_plugin(void)
{
	return &plugin_mon;
}


