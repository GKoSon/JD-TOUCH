/*
 @Time : 2021/2/5 10:52 AM
 @Author : chenye
 @File : sync
 @Software : GoLand
 @Remark : 
*/

package maps

// topic /star_line/server/syncFace/(设备code)
type FaceSync struct {
	DeviceCode 		string 			`json:"deviceCode"`			//	设备编号
	TaskID			string 			`json:"taskID"`				//	会话ID
	Data 	struct{
		UserID 			int64 			`json:"userID"`				//	用户ID
		Fea64 			string 			`json:"fea"`				//	特征值base64
		Uid 			int 			`json:"uid"`				//	第几张
		AuthStartTime 	int64 			`json:"authStartTime"`		//	授权开始时间
		AuthEndTime 	int64			`json:"authEndTime"`		//	授权结束时间
		PicUrl 			string 			`json:"picUrl"`				//	图片地址
		TimeStamp 		int64			`json:"timeStamp"`			//	记录产生时间
	} `json:"data"`
}

// topic /star_line/server/delFace/(设备code)
type FaceDelSync struct {
	DeviceCode 		string 			`json:"deviceCode"`			//	设备编号
	TaskID			string 			`json:"taskID"`				//	会话ID
	Data  	struct{
		UserID 			int64 			`json:"userID"`				//	用户ID
		Uid 			int 			`json:"uid"`				//	第几张
		TimeStamp 		int64			`json:"timeStamp"`			//	记录产生时间
	} `json:"data"`
}

//touch需要实现，黑白名单下发
// topic /star_line/server/syncFilterItem/(设备code)
type FilterItemSync struct {
	DeviceCode 		string 			`json:"deviceCode"`			//	设备编号
	TaskID			string 			`json:"taskID"`				//	会话ID
	Data 	struct{
		CardNo 			string 			`json:"cardNo"`				//	卡号
		AuthEndTime 	int64			`json:"authEndTime"`		//	授权截止时间
		FilterType		int 			`json:"filterType"`			//	1: 新增黑名单，2: 删除黑名单 3: 新增白名单，4: 取消白名单
		TimeStamp 		int64			`json:"timeStamp"`			//	记录产生时间
	} `json:"data"`

}

//touch需要实现，设备控制命令
// topic /star_line/server/deviceControl/(设备code)
type DeviceControlSync struct {
	DeviceCode 		string 			`json:"deviceCode"`			//	设备编号
	TaskID			string 			`json:"taskID"`				//	会话ID
	Data 	struct{
		PhoneNo 	string 			`json:"phoneNo"`			//	手机号
		Type 		int 			`json:"type"`				//	控制命令类型, 1-表示开，2-表示关, 3-常开
	} `json:"data"`
}

// topic /star_line/server/syncSip/(设备code)
type SipSync struct {
	TaskID			string 			`json:"taskID"`				//	会话ID
	DeviceCode 		string 			`json:"deviceCode"`			//	设备编号
	Data 	struct{
		SipNo 			string 			`json:"sipNo"`				//	sip号码
		BuildingNo 		string 			`json:"buildingNo"`			//	楼栋号
		UnitNo 			string 			`json:"unitNo"`				//	单元号
		HouseNo			string 			`json:"houseNo"`			//	房间号
		TimeStamp 		int64			`json:"timeStamp"`			//	记录产生时间
	} `json:"data"`

}

//touch需要实现，同步组code
// topic /star_line/server/syncGroup/(设备code)
type GroupSync struct {
	TaskID			string 			`json:"taskID"`				//	会话ID
	DeviceCode 		string 			`json:"deviceCode"`			//	设备编号
	Data 	struct{
		Md5Str 			string 			`json:"md5str"`				//	整个groupCodeList的md5值
		GroupList 		[]string 		`json:"groupList"`			//	组code
		TimeStamp 		int64			`json:"timeStamp"`			//	记录产生时间
	} `json:"data"`

}

//touch需要实现，校时
// topic /star_line/server/timeCalibration
type TimeCalibration struct {
	TaskID			string 			`json:"taskID"`				//	会话ID
	Data 	struct{
		TimeStamp 		int64 			`json:"timeStamp"`			// 时间戳
	} `json:"data"`

}

//touch需要实现，ota升级
// topic /star_line/server/otaDown/(设备code)
type OtaDown struct {
	TaskID 			string 		`json:"taskID"`				//	会话ID
	Data 	struct{
		FileUrl 	string 		`json:"fileUrl"`			//	文件存放路径
		Md5Str		string 		`json:"md5Str"`				//	文件md5值
	} `json:"data"`
}

//touch需要实现，命令接收状态
// topic /star_line/client/ack/(设备code)
type SyncResponse struct {
	TaskID			string 			`json:"taskID"`				//	会话ID
	StatusCode 		int 			`json:"statusCode"`			//	状态码 0-成功 1-失败
}

//touch需要实现，心跳
// topic /star_line/client/keepAlive/(设备code)
type KeepAlive struct {
	DeviceCode 		string 			`json:"deviceCode"`			//	设备编号
	Status 			int 			`json:"status"`				//	状态码0 正常 1异常
	TimeStamp 		int64			`json:"timeStamp"`			//	记录产生时间
}

// topic /star_line/client/filterSync/(设备code)
type SyncItem struct {
	DeviceCode 		string 			`json:"deviceCode"`			//	设备编号
	TimeStamp 		int64			`json:"timeStamp"`			//	记录产生时间
}

// topic /star_line/client/sipSync/(设备code)
type SyncSip struct {
	DeviceCode 		string 			`json:"deviceCode"`			//	设备编号
	TimeStamp 		int64			`json:"timeStamp"`			//	记录产生时间
}

// topic /star_line/client/faceSync/(设备code)
type SyncFace struct {
	DeviceCode 		string 			`json:"deviceCode"`			//	设备编号
	TimeStamp 		int64			`json:"timeStamp"`			//	记录产生时间
}