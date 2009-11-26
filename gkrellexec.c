/****************************************************************************/
/**
 * \file
 * \brief  A gkrellm plugin that displays the exit status of arbitrary shell
 * comands.
 */
/****************************************************************************/

#include <sys/sysinfo.h>
#include <sys/wait.h>

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
		struct {
			char cmdline[256];
			int timeout;
		}
		cfg;
		struct {
			pid_t pid;
			enum {
				PROC_OK,
				PROC_ERR,
				PROC_TIMEOUT
			}
			last;
			int timedout;
			int waitstatus;
			long uptstart;
		}
		sts;
		struct {
			GtkWidget *cmdline;
			GtkWidget *timeout;
			GkrellmDecal *decaltext;
		}
		widget;
	}
	proc[GKRELLEXEC_PROCESSES_MAXNUM];
	GkrellmPanel *panel;
	gint style_id;
	GkrellmMonitor *plugin_mon;
}
GkrExec;

/****************************************************************************/

/**
 * \brief  Returns kernel's uptime.
 * \return Uptime in seconds.
 */
static long uptime(void)
{
	struct sysinfo si;
	sysinfo(&si);
	return si.uptime;
}

/****************************************************************************/

static void update_plugin(void)
{
	int i;
	int status;
	long upt;
	GkrellmTicks *t = gkrellm_ticks();

	/* Only update once a second. */
	if (t->second_tick == 0)
		return;

	upt = uptime();

	for (i = 0; i < NMEMB(GkrExec.proc); i++) {
		/* Disabled */
		if (! GkrExec.proc[i].cfg.cmdline[0])
			continue;

		/* Not running, run: */
		if (GkrExec.proc[i].sts.pid == 0) {
			GkrExec.proc[i].sts.pid = fork();
			switch(GkrExec.proc[i].sts.pid) {
				case -1:
					/* Error */
					GkrExec.proc[i].sts.pid = 0;
					GkrExec.proc[i].sts.last = PROC_ERR;
					break;
				case 0:
					/* Child */
					execl("/bin/sh", "/bin/sh", "-c", GkrExec.proc[i].cfg.cmdline, NULL);
					exit(1);
				default:
					/* gkrellexec */
					GkrExec.proc[i].sts.uptstart = uptime();
			}
			continue;
		}

		/* Running, check status: */
		switch(waitpid(GkrExec.proc[i].sts.pid, &status, WNOHANG)) {
			case -1:
				/* Error */
				GkrExec.proc[i].sts.pid = 0;
				GkrExec.proc[i].sts.last = PROC_ERR;
				break;
			case 0:
				/* No change, timeout? */
				if (upt - GkrExec.proc[i].sts.uptstart > GkrExec.proc[i].cfg.timeout) {
					/* Timeout */
					kill(GkrExec.proc[i].sts.pid, SIGKILL);
					waitpid(GkrExec.proc[i].sts.pid, &status, 0);
					GkrExec.proc[i].sts.pid = 0;
					GkrExec.proc[i].sts.last = PROC_TIMEOUT;
				}
				break;
			default:
				/* Exited */
				if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
					GkrExec.proc[i].sts.last = PROC_OK;
				else
					GkrExec.proc[i].sts.last = PROC_ERR;
				GkrExec.proc[i].sts.waitstatus = status;
				GkrExec.proc[i].sts.pid = 0;
				break;
		}
	}

	for (i = 0; i < NMEMB(GkrExec.proc); i++) {
		char last = '?';
		char tmp[300];
		gkrellm_draw_decal_text(GkrExec.panel, GkrExec.proc[i].widget.decaltext, "", -1);
		switch(GkrExec.proc[i].sts.last) {
			case PROC_OK:      last = 'O'; break;
			case PROC_ERR:     last = 'E'; break;
			case PROC_TIMEOUT: last = 'T'; break;
		}
		snprintf(tmp, sizeof(tmp), "%c %s", last, GkrExec.proc[i].cfg.cmdline);
		gkrellm_draw_decal_text(GkrExec.panel, GkrExec.proc[i].widget.decaltext, tmp, -1);
	}

	gkrellm_draw_panel_layers(GkrExec.panel);
}

/****************************************************************************/

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

	for (i = 0; i < NMEMB(GkrExec.proc); i++) {
		GkrExec.proc[i].widget.decaltext = gkrellm_create_decal_text(GkrExec.panel, "Ayl0", ts, style, -1, prevy + prevh + 2, -1);
		prevy = GkrExec.proc[i].widget.decaltext->y;
		prevh = GkrExec.proc[i].widget.decaltext->h;
		gkrellm_decal_on_top_layer(GkrExec.proc[i].widget.decaltext, TRUE);
	}

	gkrellm_panel_configure(GkrExec.panel, NULL, style);
	gkrellm_panel_create(vbox, GkrExec.plugin_mon, GkrExec.panel);
	gkrellm_draw_panel_layers(GkrExec.panel);
}

/****************************************************************************/

/**
 * \brief  Create an option in config screen.
 * \param  parent Parent vbox.
 * \param  name Option name.
 * \param  size Max length of user text.
 * \param  starttext Initial text.
 * \return The option widget.
 */
static GtkWidget* create_option(GtkWidget *parent, const char *name, int size, const char *starttext)
{
	GtkWidget *hbox;
	GtkWidget *label;
	GtkWidget *rv;

	hbox = gtk_hbox_new(FALSE, 10);
	gtk_box_pack_start(GTK_BOX(parent), hbox, FALSE, TRUE, 5);
	label = gtk_label_new(name);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	rv = gtk_entry_new_with_max_length(size);
	gtk_box_pack_start(GTK_BOX(hbox), rv, FALSE, TRUE, 0);
	gtk_entry_set_text(GTK_ENTRY(rv), starttext);

	return rv;
}


static void create_plugin_tab(GtkWidget * tab_vbox)
{
	GtkWidget *tabs;
	GtkWidget *about;
	GtkWidget *label;
	int i;

	tabs = gtk_notebook_new();
	gtk_notebook_set_tab_pos(GTK_NOTEBOOK(tabs), GTK_POS_TOP);
	gtk_box_pack_start(GTK_BOX(tab_vbox), tabs, TRUE, TRUE, 0);

	for (i = 0; i < NMEMB(GkrExec.proc); i++) {
		GtkWidget *vbox0;
		char tmp[256];

		/* Tab: */
		snprintf(tmp, sizeof tmp, "Process %d", i + 1);
		vbox0 = gkrellm_gtk_framed_notebook_page(tabs, tmp);

		GkrExec.proc[i].widget.cmdline = create_option(vbox0, "Command line:", 256, GkrExec.proc[i].cfg.cmdline);

		sprintf(tmp, "%d", GkrExec.proc[i].cfg.timeout);
		GkrExec.proc[i].widget.timeout = create_option(vbox0, "Timeout:", 10, tmp);
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

	for (i = 0; i < NMEMB(GkrExec.proc); i++) {
		snprintf(GkrExec.proc[i].cfg.cmdline, sizeof(GkrExec.proc[i].cfg.cmdline), "%s", gkrellm_gtk_entry_get_text(&GkrExec.proc[i].widget.cmdline));
		GkrExec.proc[i].cfg.timeout = atoi(gkrellm_gtk_entry_get_text(&GkrExec.proc[i].widget.timeout));
	}

#if 0
	/* Re-layout pannel */
	gkrellm_panel_destroy(amiconn_panel);
	CreatePlugin(amiconn_vbox, TRUE);
#endif
}

/****************************************************************************/

static void save_plugin_config(FILE *f)
{
	int i;

	for (i = 0; i < NMEMB(GkrExec.proc); i++) {
		if (GkrExec.proc[i].cfg.cmdline[0] == 0)
			continue;
		fprintf(f, CONFIG_KEYWORD " cmdline %d %s\n", i, GkrExec.proc[i].cfg.cmdline);
		fprintf(f, CONFIG_KEYWORD " timeout %d %d\n", i, GkrExec.proc[i].cfg.timeout);
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
			snprintf(GkrExec.proc[i].cfg.cmdline, sizeof(GkrExec.proc[i].cfg.cmdline), "%s", item);
		if (strcmp(config, "timeout") == 0)
			sscanf(item, "%d\n", &GkrExec.proc[i].cfg.timeout);
	}
}

/****************************************************************************/

static GkrellmMonitor plugin_mon =
{
	"gkrellexec",        /* Name, for config tab.        */
	0,                   /* Id,  0 if a plugin           */
	create_plugin,       /* The create_plugin() function */
	update_plugin,       /* The update_plugin() function */
	create_plugin_tab,   /* The create_plugin_tab() config function */
	apply_plugin_config, /* The apply_plugin_config() function      */
	save_plugin_config,  /* The save_plugin_config() function  */
	load_plugin_config,  /* The load_plugin_config() function  */
	CONFIG_KEYWORD,      /* config keyword                     */
	NULL,                /* Undefined 2  */
	NULL,                /* Undefined 1  */
	NULL,                /* Undefined 0  */
	PLUGIN_PLACEMENT,    /* Insert plugin before this monitor.       */
	NULL,                /* Handle if a plugin, filled in by GKrellM */
	NULL                 /* path if a plugin, filled in by GKrellM   */
};


GkrellmMonitor *gkrellm_init_plugin(void)
{
	GkrExec.style_id = gkrellm_add_meter_style(&plugin_mon, CONFIG_KEYWORD);
	GkrExec.plugin_mon = &plugin_mon;
	return &plugin_mon;
}


