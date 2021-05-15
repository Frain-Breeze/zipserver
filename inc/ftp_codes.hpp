#pragma once

// 1xx PP = positive preliminary = action initiated, another reply is coming
// 2xx PC = positive completion = action finished
// 3xx PI = positive intermediate = action waiting for another command with information
// 4xx NT = negative temporary = action didn't take place, but you can try again later
// 5xx NP = negative permanent = action didn't take place and you shouldn't send the same command again. (the command might be malformed, in which case you can retry after a fix)
// 6xx PS = protected secure = for security stuff

// x0x _S_ = syntax = anything related to syntax stuff
// x1x _I_ = information = status, help, etc
// x2x _C_ = connection = data stuff (not files)
// x3x _A_ = authentication = login process etc
// x5x _F_ = filesystem = related to file operations

/*enum class FTPCode : uint16_t {
	PP_S_INITIATED = 100,
	//110
	//120
	//125
	PP_F_STATUS_OK = 150,

	PC_S_SUCCESS = 200, //general ok?
	//202
	//211
	//212
	//213
	//214
	//215
	PC_C_READY_NEW_USER = 220,
	//221
	//225
	//226
	PC_C_ENTER_PASV = 227,
	//227 long pasv
	//229 extended pasv
	PC_A_LOGGED_IN = 230, //user logged in
	//231
	//232
	//234
	//250
	PC_F_NAME_CREATED = 257, //used as PWD return

	PI_S_ON_HOLD = 300,
	PI_A_NEED_PASSWORD = 331,
	PI_A_NEED_ACCOUNT = 332,
	PI_F_PENDING_FURTHER_INFO = 350,

	//400
	//421
	//425
	//426
	//430
	//434
	//450
	//451
	//452

	//500
	//501
	//502
	//503
	//504
	//530
	//532
	//534
	//550
	//551
	//552
	//553

	//600
	//631
	//632
	//633
};*/

enum class FTPCode : uint16_t {
	COMMAND_NOT_IMPLEMENTED = 202,
	READY = 220, //used directly after establishing connection
	NEED_PASSWORD = 331, //used after getting username
	GREETING = 230, //user logged in
	UNKNOWN_COMMAND = 501,
	PATHNAME_CREATED = 257, //for pwd return
	OKAY = 200,
	ENTERING_PASV_MODE = 227,
	ACTION_NOT_TAKEN_UNAVAILABLE = 550, // used for denying AUTH requests, cuz apparently "unknown command" is not acceptable?
	CLOSING_DATA_CONNECTION = 226,
	GOING_TO_OPEN_DATA_CONNECTION = 150,
	FILE_STATUS = 213,
	SYSTEM_STATUS = 211,
	SYNTAX_ERROR_IN_COMMAND = 501,
};