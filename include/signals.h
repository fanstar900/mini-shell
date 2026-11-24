#pragma once

// Makes the shell ignore the interactive job-control signals
// (SIGINT, SIGQUIT, SIGTSTP, SIGTTIN, SIGTTOU) so only the
// foreground job receives them.
void setupSignalHandlers();

// Restores the default disposition of those signals; called in
// child processes before exec.
void resetChildSignals();
