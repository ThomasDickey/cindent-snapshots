void
failed(mode_t *_t)
{
    /* *INDENT-EQLS* */
    parser_state_tos->next                = 0;
  parser_state_tos->tos = 0;
  parser_state_tos->paren_indents_size = 1;
  parser_state_tos->p_stack[0] = stmt;

  parser_state_tos->last_nl = true;
    perror(_t);
    exit(BADEXIT);
}
