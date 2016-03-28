%{
    #include "global.h"
    int yylex(void);
    void yyerror ();
      
    int offset, len, commandDone;
%}

%token STRING

%%
line            :   /* empty */					
                    |command                        {   execute();  commandDone = 1;   }
;

command         :   fgCommand
                    |fgCommand '&'
;

fgCommand       :   simpleCmd
;

simpleCmd       :   progInvocation inputRedirect outputRedirect
;

progInvocation  :   STRING args
;

inputRedirect   :   /* empty */
                    |'<' STRING
;

outputRedirect  :   /* empty */
                    |'>' STRING
;

args            :   /* empty */
                    |STRING
;

%%
/****************************************************************
                  �ʷ���������
****************************************************************/

/****************************************************************
                  ������Ϣִ�к���
****************************************************************/
void yyerror()
{
    printf("������������ȷ�����������룡\n");
}
/****************************************************************
                  main������
****************************************************************/
int main(int argc, char** argv) {
    int i;
    char c;

    init(); //��ʼ������
    commandDone = 0;
    
    printf("yourname@computer:%s$ ", get_current_dir_name()); //��ӡ��ʾ����Ϣ

    while(1){
        i = 0;
        while((c = getchar()) != '\n'){ //����һ������
            inputBuff[i++] = c;
        }
        inputBuff[i] = '\0';

        len = i;
        offset = 0;
        yy_switch_to_buffer(yy_scan_string(inputBuff));
        yyparse(); //�����﷨�����������ú�����yylex()�ṩ��ǰ����ĵ��ʷ���

        if(commandDone == 1){ //�����Ѿ�ִ����ɺ������ʷ��¼��Ϣ
            commandDone = 0;
            addHistory(inputBuff);
        }
        
        printf("yourname@computer:%s$ ", get_current_dir_name()); //��ӡ��ʾ����Ϣ
     }

    return (EXIT_SUCCESS);
}
