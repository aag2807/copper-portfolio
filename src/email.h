#ifndef EMAIL_H
#define EMAIL_H

typedef struct
{
    int ok;            // 1 on success (HTTP 2xx from Resend), 0 otherwise.
    int status_code;   // HTTP status returned by Resend (0 on transport error).
    char message[512]; // human-readable status or curl error.
} EmailResult;

void email_init(void);
void email_cleanup(void);
EmailResult email_send_via_resend(const char* api_key, const char* from, const char* to, const char* reply_to, const char* subject, const char* html);

#endif // !EMAIL_H
