/*
 * Copyright (c) 2014 Barret Rennie
 * Rights to this code are as documented in LICENSE
 *
 * adminserv: A service for atheme offering "psuedo-oper" services.
 *
 * The whois module offers extended WHOIS capabilities to users with the
 * adminserv:whois privilege.
 */

#include "atheme.h"

#include "adminserv.h"

DECLARE_MODULE_V1(
	ADMINSERV_WHOIS_MODULE_NAME, false, _modinit, _moddeinit,
	"0.1",
	"Barret Rennie <https://www.brennie.ca>"
);

/* The size of the buffer for transmitting channel data to the user. This is 
 * set to 81 (80 characters + \0) so that we do no linewrap on narrow terminals.
 */
#define CHANNEL_BUFSIZE 81

static void as_cmd_whois(sourceinfo_t *si, int parc, char *parv[]);

static command_t as_whois =
{
	"WHOIS", "Get information about a user.",
	ADMINSERV_CAN_WHOIS, 1, as_cmd_whois, { .path = "adminserv/whois" }
};

static void as_cmd_whois(sourceinfo_t *si, int parc, char *parv[])
{
	char *target_name = parv[0];
	user_t *target = NULL;

	char buf[CHANNEL_BUFSIZE] = "";
	size_t buf_len = strlen(buf);

	if (!si->su)
		command_fail(si, fault_noprivs, _("\2%s\2 can only be excuted via IRC."), "WHOIS");

	else if (!target_name)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "WHOIS");
		command_fail(si, fault_needmoreparams, _("Syntax: WHOIS <target>"));
	}
	else if ((target = user_find_named(target_name)) == NULL)
		command_fail(si, fault_nosuch_target, _("No such user: %s"), target_name);

	else
	{
		command_success_nodata(si, _("WHOIS information for: %s!%s@%s"), target_name,
			target->user, target->host);

		command_success_nodata(si, _("Real name: %s"), target->gecos);
		command_success_nodata(si, _("Cloaked Host: %s"), target->chost);
		command_success_nodata(si, _("Visible Host: %s"), target->vhost);	
		command_success_nodata(si, _("IP Address: %s"), target->ip);
		command_success_nodata(si, _("Connected to: %s"), target->server->name);

		/* We send 80-character wide lists of channels to the client if the target
		 * has joined channels.
		 */
		if (target->channels.count)
		{

			char buf[BUFSIZE];
			size_t buf_len;

			mowgli_node_t *node;
			
			strncpy(buf, "Channels:", CHANNEL_BUFSIZE);
			buf_len = strlen(buf);

			MOWGLI_LIST_FOREACH(node, target->channels.head)
			{
				const chanuser_t *cu = node->data;
				const char *chan_name = cu->chan->name;

				size_t chan_len = strlen(chan_name);

				/* Ensure we can fit the channel name in the buffer */
				if (buf_len + 1 + chan_len >= CHANNEL_BUFSIZE)
				{
					command_success_nodata(si, "%s", buf);
					strncpy(buf, "Channels:", CHANNEL_BUFSIZE);
					buf_len = strlen(buf);
				}

				strncat(buf, " ", 2);
				strncat(buf, chan_name, CHANNEL_BUFSIZE - buf_len);
				buf_len += chan_len;
			}
		
			command_success_nodata(si, "%s", buf);
			buf[0] = '\0';
			buf_len = 0;
		}
		else
			command_success_nodata(si, _("User is not joined to any channels."));

		if (target->myuser)
			command_success_nodata(si, _("User has identified for account %s"),
				target->myuser->ent.name);
		else
			command_success_nodata(si, _("User has not identified."));

		if (is_admin(target))
			command_success_nodata(si, _("User is an administrator."));
		else if (is_ircop(target))
			command_success_nodata(si, _("User is an operator."));

		command_success_nodata(si, _("End of WHOIS."));


		logcommand(si, CMDLOG_DO, "WHOIS: \2%s\2", target_name);
	}
}

void _modinit(module_t *module)
{
	service_named_bind_command(ADMINSERV, &as_whois);
}

void _moddeinit(module_unload_intent_t intent)
{
	service_named_unbind_command(ADMINSERV, &as_whois);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
