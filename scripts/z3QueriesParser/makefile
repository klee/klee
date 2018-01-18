###################
# TOP DOWN PARSER #
###################
CC=gcc

###############
# DIRECTORIES #
###############
BASEDIR           = $(shell pwd)
LEXER_DIR         = $(BASEDIR)/FOLDER_0_Flex
LEXER_USER_DIR    = $(BASEDIR)/FOLDER_1_LEXER
SOURCE_FILES_DIR  = $(BASEDIR)/FOLDER_2_SOURCE_FILES
HEADER_FILES_DIR  = $(BASEDIR)/FOLDER_3_HEADER_FILES
PROG_DIR          = $(BASEDIR)/FOLDER_4_PARSER
INPUT_DIR         = $(BASEDIR)/FOLDER_5_INPUT
OUTPUT_DIR        = $(BASEDIR)/FOLDER_6_OUTPUT
DEBUG_SCRIPTS_DIR = $(BASEDIR)/FOLDER_7_GDB_SCRIPTS
LEXER_SOURCE_DIR  = $(LEXER_DIR)/SOURCE_FILES
LEXER_HEADER_DIR  = $(LEXER_DIR)/HEADER_FILES
LEXER_OBJECT_DIR  = $(LEXER_DIR)/OBJECT_FILES

###############
# LEXER FLAGS #
###############
LEXER_USER_FLAGS = -o$(LEXER_USER_DIR)/Lexer.c

#####################
# LEXER INPUT FILES #
#####################
LEXER_USER_INPUT = $(LEXER_USER_DIR)/LEX_FILE.lex

#########
# LEXER #
#########
LEXER_FLAGS           = -I$(LEXER_HEADER_DIR) -w
LEXER_PROGRAM         = $(LEXER_DIR)/MyFlex 
LEXER_SOURCE_FILES   := $(wildcard  $(LEXER_SOURCE_DIR)/*.c)
LEXER_HEADER_FILES   := $(wildcard  $(LEXER_HEADER_DIR)/*.h)
LEXER_OBJECT_FILES   := $(addprefix $(LEXER_OBJECT_DIR)/,$(LEXER_SOURCE_FILES,.c=.o))

###################
# PARSER #
###################
PARSER_FLAGS         = -I$(HEADER_FILES_DIR) -w
PARSER_PROGRAM       = $(PROG_DIR)/PARSER
PARSER_SOURCE_FILES := $(wildcard  $(SOURCE_FILES_DIR)/*.c)
PARSER_HEADER_FILES := $(wildcard  $(HEADER_FILES_DIR)/*.h)

################
# INPUT/OUTPUT #
################
INPUT  = $(INPUT_DIR)/Input.txt
OUTPUT = $(OUTPUT_DIR)/Output.txt

##########################
# ALL LEXER(S) PARSER(S) #
##########################
all:
	@echo "\n"
	@echo "*****************"
	@echo "* Clean All ... *"
	@echo "*****************"
	@echo "\n"
	rm -rf $(LEXER_PROGRAM) 
	rm -rf $(PARSER_PROGRAM) 
	rm -rf $(SOURCE_FILES_DIR)/Lexer.c
	@echo "\n"
	@echo "******************"
	@echo "* Build Flex ... *"
	@echo "******************"
	@echo "\n"	
	$(CC) $(LEXER_FLAGS) $(LEXER_SOURCE_FILES) -o $(LEXER_PROGRAM)
	@echo "\n"
	@echo "************************"
	@echo "* Run All Lexer(s) ... *"
	@echo "************************"
	@echo "\n"
	$(LEXER_PROGRAM) $(LEXER_USER_FLAGS) $(LEXER_USER_INPUT)
	@echo "\n"
	@echo "**************************************************"
	@echo "* Move Generated Lexer(s) to destined folder ... *"
	@echo "**************************************************"
	@echo "\n"	
	mv $(LEXER_USER_DIR)/Lexer.c $(SOURCE_FILES_DIR)
	@echo "\n"
	@echo "********************"
	@echo "* Build PARSER ... *"
	@echo "********************"
	@echo "\n"	
	$(CC) $(PARSER_FLAGS) $(PARSER_SOURCE_FILES) -o $(PARSER_PROGRAM)
	@echo "\n"
	@echo "**********************"
	@echo "* Execute PARSER ... *"
	@echo "**********************"
	@echo "\n"	
	$(PARSER_PROGRAM) $(INPUT) $(OUTPUT)
	@echo "\n"
	@echo "***************************************"
	@echo "* Extract Value from Abstract Buffers *"
	@echo "***************************************"
	@echo "\n"
	@echo "))" >> ${INPUT_DIR}/AB_All_Versions.txt
	@sed -i 's/(reset)/ /g' ${OUTPUT}
	@cat ${INPUT_DIR}/AB_All_Versions.txt >> ${OUTPUT}

