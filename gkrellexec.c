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

#define PROGRAM_NAME "gkrellexec"
#define PROGRAM_VERSION "1.0.4"

#define CONFIG_KEYWORD PROGRAM_NAME
#define GKRELLEXEC_VERSION PROGRAM_VERSION
#define PLUGIN_PLACEMENT MON_MAIL
#define GKRELLEXEC_PROCESSES_MAXNUM 10

#define NMEMB(x) ((sizeof(x)) / (sizeof(*x)))

static gchar AboutText[] = "GKrellM Exec version " GKRELLEXEC_VERSION
                           "\n\n\n"
                           "Copyright (C) 2009 by Leandro Penz\n"
                           "lpenz@lpenz.org\n"
                           "http://github.com/lpenz/gkrellexec\n"
                           "Released under the GPL\n";

static gchar *HelpText[] = {
    "<h>" CONFIG_KEYWORD "\n\n",
    "gkellexec displays one line per process, with its name and exit "
    "status\n\n",
    "<b>Exit codes\n", "- O: ok\n", "- E: error\n", "- T: timeout\n\n",
    "<b>Configuration options\n",
    "- Name: name of process, displayed in panel; clear it to disable "
    "process.\n",
    "- Command line: executed with /bin/sh -c <cmdline>; its exit status is "
    "shown in panel.\n",
    "- Timeout: after the specified amount of seconds, the process is killed "
    "and T shown.\n",
    "- Sleep after success: amount of seconds to sleep after an Ok exit "
    "code.\n",
    "- Sleep after failure: amount of seconds to sleep after a failure.\n\n",
    "<b>Examples\n",
    "Use \"ping -c1 <host>\" to check if a host is alive.\n"
    "Use ifconfig with grep to check if an interface is up.\n"
    "Use grep with /proc files to check many things.\n"};

struct proc_status {
    char repr;
};
struct proc_status ProcStatusUnknown = {'?'};
struct proc_status ProcStatusOk = {'O'};
struct proc_status ProcStatusError = {'E'};
struct proc_status ProcStatusTimeout = {'T'};

typedef struct proc_status *proc_status_t;

static struct {
    struct {
        struct {
            char name[256];
            char cmdline[256];
            int timeout;
            int sleepok;
            int sleeperr;
        } cfg;
        struct {
            pid_t pid;
            proc_status_t last;
            int timedout;
            int waitstatus;
            long uptstart;
            long uptend;
        } sts;
        struct {
            GtkWidget *name;
            GtkWidget *cmdline;
            GtkWidget *timeout;
            GtkWidget *sleepok;
            GtkWidget *sleeperr;
            GkrellmDecal *decaltext;
        } widget;
    } proc[GKRELLEXEC_PROCESSES_MAXNUM];
    GtkWidget *vbox;
    GkrellmPanel *panel;
    gint style_id;
    GkrellmMonitor *plugin_mon;
    GkrellmTextstyle *textstyle;
    GkrellmStyle *style;
} GkrExec;

/****************************************************************************/

/**
 * \brief  Returns kernel's uptime.
 * \return Uptime in seconds.
 */
static long uptime(void) {
    struct sysinfo si;
    sysinfo(&si);
    return si.uptime;
}

/****************************************************************************/

static void update_plugin(void) {
    int i;
    int status;
    long upt;
    GkrellmTicks *t = gkrellm_ticks();

    /* Only update once a second. */
    if (t->second_tick == 0) return;

    upt = uptime();

    for (i = 0; i < NMEMB(GkrExec.proc); i++) {
        /* Disabled */
        if (!GkrExec.proc[i].cfg.name[0]) continue;

        /* Not running, run: */
        if (GkrExec.proc[i].sts.pid == 0 &&
            (
                /* Unknown */
                GkrExec.proc[i].sts.last == &ProcStatusUnknown
                /* Ok and sleepok gone */
                ||
                (GkrExec.proc[i].sts.last == &ProcStatusOk &&
                 uptime() - GkrExec.proc[i].sts.uptend >
                     GkrExec.proc[i].cfg.sleepok)
                /* Error or timeout and sleeperr gone */
                ||
                ((GkrExec.proc[i].sts.last == &ProcStatusError ||
                  GkrExec.proc[i].sts.last == &ProcStatusTimeout) &&
                 uptime() - GkrExec.proc[i].sts.uptend >
                     GkrExec.proc[i].cfg.sleeperr))) {
            GkrExec.proc[i].sts.pid = fork();
            switch (GkrExec.proc[i].sts.pid) {
                case -1:
                    /* Error */
                    GkrExec.proc[i].sts.pid = 0;
                    GkrExec.proc[i].sts.last = &ProcStatusError;
                    break;
                case 0:
                    /* Child */
                    execl("/bin/sh", "/bin/sh", "-c",
                          GkrExec.proc[i].cfg.cmdline, NULL);
                    exit(1);
                default:
                    /* gkrellexec */
                    GkrExec.proc[i].sts.uptstart = uptime();
            }
            continue;
        }

        /* Not supposed to run yet. */
        if (GkrExec.proc[i].sts.pid == 0) continue;

        /* Running, check status: */
        switch (waitpid(GkrExec.proc[i].sts.pid, &status, WNOHANG)) {
            case -1:
                /* Error */
                GkrExec.proc[i].sts.pid = 0;
                GkrExec.proc[i].sts.last = &ProcStatusError;
                GkrExec.proc[i].sts.uptend = uptime();
                break;
            case 0:
                /* No change, timeout? */
                if (upt - GkrExec.proc[i].sts.uptstart >
                    GkrExec.proc[i].cfg.timeout) {
                    /* Timeout */
                    kill(GkrExec.proc[i].sts.pid, SIGKILL);
                    waitpid(GkrExec.proc[i].sts.pid, &status, 0);
                    GkrExec.proc[i].sts.pid = 0;
                    GkrExec.proc[i].sts.last = &ProcStatusTimeout;
                    GkrExec.proc[i].sts.uptend = uptime();
                }
                break;
            default:
                /* Exited */
                if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
                    GkrExec.proc[i].sts.last = &ProcStatusOk;
                else
                    GkrExec.proc[i].sts.last = &ProcStatusError;
                GkrExec.proc[i].sts.waitstatus = status;
                GkrExec.proc[i].sts.pid = 0;
                GkrExec.proc[i].sts.uptend = uptime();
                break;
        }
    }

    for (i = 0; i < NMEMB(GkrExec.proc); i++) {
        char tmp[60];

        if (!GkrExec.proc[i].cfg.name[0]) continue;

        gkrellm_draw_decal_text(GkrExec.panel, GkrExec.proc[i].widget.decaltext,
                                "", -1);
        snprintf(tmp, sizeof(tmp), "%c %s", GkrExec.proc[i].sts.last->repr,
                 GkrExec.proc[i].cfg.name);
        gkrellm_draw_decal_text(GkrExec.panel, GkrExec.proc[i].widget.decaltext,
                                tmp, -1);
    }

    gkrellm_draw_panel_layers(GkrExec.panel);
}

/****************************************************************************/

static void create_plugin(GtkWidget *vbox, gint firstcreate) {
    int i;
    gint prevy = 0;
    gint prevh = 0;

    if (firstcreate) GkrExec.panel = gkrellm_panel_new0();

    GkrExec.vbox = vbox;
    GkrExec.style = gkrellm_meter_style(GkrExec.style_id);
    GkrExec.textstyle = gkrellm_meter_textstyle(GkrExec.style_id);

    for (i = 0; i < NMEMB(GkrExec.proc); i++) {
        memset(&GkrExec.proc[i].sts, 0, sizeof(GkrExec.proc[i].sts));
        if (!GkrExec.proc[i].cfg.name[0]) continue;
        GkrExec.proc[i].widget.decaltext =
            gkrellm_create_decal_text(GkrExec.panel, "Ayl0", GkrExec.textstyle,
                                      GkrExec.style, -1, prevy + prevh + 2, -1);
        prevy = GkrExec.proc[i].widget.decaltext->y;
        prevh = GkrExec.proc[i].widget.decaltext->h;
        gkrellm_decal_on_top_layer(GkrExec.proc[i].widget.decaltext, TRUE);
        GkrExec.proc[i].sts.last = &ProcStatusUnknown;
    }

    gkrellm_panel_configure(GkrExec.panel, NULL, GkrExec.style);
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
static GtkWidget *create_option(GtkWidget *parent, const char *name, int size,
                                const char *starttext) {
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

static void create_plugin_tab(GtkWidget *tab_vbox) {
    GtkWidget *tabs;
    GtkWidget *text;
    GtkWidget *label;
    GtkWidget *vbox;
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

        GkrExec.proc[i].widget.name =
            create_option(vbox0, "Name:", 256, GkrExec.proc[i].cfg.name);

        GkrExec.proc[i].widget.cmdline = create_option(
            vbox0, "Command line:", 256, GkrExec.proc[i].cfg.cmdline);

        sprintf(tmp, "%d", GkrExec.proc[i].cfg.timeout);
        GkrExec.proc[i].widget.timeout =
            create_option(vbox0, "Timeout:", 10, tmp);

        sprintf(tmp, "%d", GkrExec.proc[i].cfg.sleepok);
        GkrExec.proc[i].widget.sleepok =
            create_option(vbox0, "Sleep after success:", 10, tmp);

        sprintf(tmp, "%d", GkrExec.proc[i].cfg.sleeperr);
        GkrExec.proc[i].widget.sleeperr =
            create_option(vbox0, "Sleep after error:", 10, tmp);
    }

    /* Help */
    vbox = gkrellm_gtk_framed_notebook_page(tabs, "Help");
    text = gkrellm_gtk_scrolled_text_view(vbox, NULL, GTK_POLICY_AUTOMATIC,
                                          GTK_POLICY_AUTOMATIC);
    for (i = 0; i < sizeof(HelpText) / sizeof(gchar *); ++i)
        gkrellm_gtk_text_view_append(text, HelpText[i]);

    /* About */
    label = gtk_label_new("About");
    text = gtk_label_new(AboutText);
    gtk_notebook_append_page(GTK_NOTEBOOK(tabs), text, label);
}

static void apply_plugin_config(void) {
    int i;

    for (i = 0; i < NMEMB(GkrExec.proc); i++) {
        snprintf(GkrExec.proc[i].cfg.name, sizeof(GkrExec.proc[i].cfg.name),
                 "%s",
                 gkrellm_gtk_entry_get_text(&GkrExec.proc[i].widget.name));
        snprintf(GkrExec.proc[i].cfg.cmdline,
                 sizeof(GkrExec.proc[i].cfg.cmdline), "%s",
                 gkrellm_gtk_entry_get_text(&GkrExec.proc[i].widget.cmdline));
        GkrExec.proc[i].cfg.timeout =
            atoi(gkrellm_gtk_entry_get_text(&GkrExec.proc[i].widget.timeout));
        GkrExec.proc[i].cfg.sleepok =
            atoi(gkrellm_gtk_entry_get_text(&GkrExec.proc[i].widget.sleepok));
        GkrExec.proc[i].cfg.sleeperr =
            atoi(gkrellm_gtk_entry_get_text(&GkrExec.proc[i].widget.sleeperr));
    }

    gkrellm_panel_destroy(GkrExec.panel);
    create_plugin(GkrExec.vbox, TRUE);
}

/****************************************************************************/

static void save_plugin_config(FILE *f) {
    int i;

    for (i = 0; i < NMEMB(GkrExec.proc); i++) {
        if (GkrExec.proc[i].cfg.name[0] == 0) continue;
        fprintf(f, CONFIG_KEYWORD " name %d %s\n", i, GkrExec.proc[i].cfg.name);
        fprintf(f, CONFIG_KEYWORD " cmdline %d %s\n", i,
                GkrExec.proc[i].cfg.cmdline);
        fprintf(f, CONFIG_KEYWORD " timeout %d %d\n", i,
                GkrExec.proc[i].cfg.timeout);
        fprintf(f, CONFIG_KEYWORD " sleepok %d %d\n", i,
                GkrExec.proc[i].cfg.sleepok);
        fprintf(f, CONFIG_KEYWORD " sleeperr %d %d\n", i,
                GkrExec.proc[i].cfg.sleeperr);
    }
}

static void load_plugin_config(gchar *arg) {
    gchar config[64], item[256];
    gint n;
    int i;

    n = sscanf(arg, "%s %d %[^\n]", config, &i, item);
    if (n == 3) {
        if (strcmp(config, "name") == 0)
            snprintf(GkrExec.proc[i].cfg.name, sizeof(GkrExec.proc[i].cfg.name),
                     "%s", item);
        if (strcmp(config, "cmdline") == 0)
            snprintf(GkrExec.proc[i].cfg.cmdline,
                     sizeof(GkrExec.proc[i].cfg.cmdline), "%s", item);
        if (strcmp(config, "timeout") == 0)
            sscanf(item, "%d\n", &GkrExec.proc[i].cfg.timeout);
        if (strcmp(config, "sleepok") == 0)
            sscanf(item, "%d\n", &GkrExec.proc[i].cfg.sleepok);
        if (strcmp(config, "sleeperr") == 0)
            sscanf(item, "%d\n", &GkrExec.proc[i].cfg.sleeperr);
    }
}

/****************************************************************************/

static GkrellmMonitor plugin_mon = {
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

GkrellmMonitor *gkrellm_init_plugin(void) {
    GkrExec.style_id = gkrellm_add_meter_style(&plugin_mon, CONFIG_KEYWORD);
    GkrExec.plugin_mon = &plugin_mon;
    return &plugin_mon;
}
