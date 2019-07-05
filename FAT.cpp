#include <iostream>
#include <cstring>
//#include <stringstream>

#include "FAT.h"

using namespace std;

#define _maxUsers 21
#define DirDepth 10            //子目录个数；
char* _virtualDiskAddr;   //虚拟磁盘的入口地坢��?
int _emptyBLOCK_Count;      //可用的BLOCK数量�?
//int _emptyFCB_Count;         //可用的FCB的数量；
FAT* _fat;//系统FAT�?
FCB* _root;   //指向root，实现绝对路径；
FCBBLOCK* _users;
FCB* current_user;//ֻ���ṩ�����ʶ
int current_usernum;//������Ȩ�ޱ�־
FCB* _current; //当前目录�?
string current_path;      //当前路径�?
/**********************************************************************/
//磁盘管理声明�?
/**********************************************************************/
//初始化虚拟磁盘；
//由于用户可能霢�要恢复之前的内存状��，扢�以不按照当下的初始化，提供if判断即可�?
void sys_initDisk() {
    _virtualDiskAddr = (char *)malloc(_virtualDiskSize * sizeof(char));   //声请空间：char是一个字节，扢�以虚拟磁盘的大小�?M�?

    _fat = new (_virtualDiskAddr)FAT();//初始为空

    /*//建立根root初始化文件目录；
    _root = new(_virtualDiskAddr + _FatBlockCount * _blockSize)FCB();
    strcpy(_root->_name, "root");
    _root->_parent = _root;
    _current = _root;
    current_path = "//";
    */
    ////_fat->fats[0]=-1!
    _fat->fats[1]=_systemUsedBlock;//用户块！！！
    _users=new(_virtualDiskAddr + _systemUsedBlock * _blockSize)FCBBLOCK();
    _users->_contentLength=1;
    _users->_blockID = _systemUsedBlock;
    _root = &(_users->_content[0]);//????????
    current_user=_root;
    current_usernum=0;
    strcpy(_root->_name, "root");//type size默认
    ////_root->_block=;????????????????????????
    _current = _root;
    current_path = "//";
/*
    //把空的FCB链接起来�?
    //char* _FCB_StartAllocation = _virtualDiskAddr + _bitMapCount * _blockSize + 64;    //64是root的FCB�?
    _emptyFCB = new(_root + 1)FCB();
    _lastFCB = _emptyFCB;
    for (int i = 1; i < _FCB_count - 1; i++)   //还剩_FCB_count-1个FCB，每�?4B�?
    {
        _lastFCB->_child = new(_lastFCB + 1)FCB();
        _lastFCB = _lastFCB->_child;
    }
    _emptyFCB_Count = _FCB_count - 1;
*/
/*    //初始化Block�?
    BLOCK* initBlock;
    BLOCK* nextBlock;
    initBlock = new(_virtualDiskAddr + _systemUsedBlock * _blockSize)BLOCK();
    initBlock->_blockID = _systemUsedBlock;        //128;
    for (int i = 1; i < _BLOCK_count; i++)
    {
        nextBlock = new(initBlock + 1)BLOCK();
        initBlock=nextBlock;
        initBlock->_blockID = _systemUsedBlock + i;
    }
*/
///这部分还霢�要吗
/*
    //把空的BLOCK链接起来�?
    _emptyBLOCK = new(_virtualDiskAddr + _systemUsedBlock * _blockSize)BLOCK();
    _emptyBLOCK->_blockID = _systemUsedBlock;        //128;
    _lastBLOCK = _emptyBLOCK;
    for (int i = 1; i < _BLOCK_count; i++)
    {
        _lastBLOCK->_nextBlock = new(_lastBLOCK + 1)BLOCK();
        _lastBLOCK = _lastBLOCK->_nextBlock;
        _lastBLOCK->_blockID = _systemUsedBlock + i;
    }
*/
    ////_emptyBLOCK_Count = _BLOCK_count;
    _emptyBLOCK_Count = _BLOCK_count-1;
}


//////////////////////////////////////////////////////////////////////////////////////////////
//把块（��虑块链）初始化为block
FileBLOCK* initFileBlock(int num) {
    FileBLOCK* head_block = new(_virtualDiskAddr + num*_blockSize)FileBLOCK();
    FileBLOCK* next_block ;
    int tmp=_fat->fats[num];
    while(tmp!=-1&&tmp!=128){
        next_block = new(_virtualDiskAddr + tmp*_blockSize)FileBLOCK();
        tmp=_fat->fats[tmp];
    }
    return head_block;
}
FCBBLOCK* initFCBBlock(int num) {//num丢�般为1
    FCBBLOCK* head_block = new(_virtualDiskAddr + num*_blockSize)FCBBLOCK();
    FCBBLOCK* next_block ;
    int tmp=_fat->fats[num];
    while(tmp!=-1&&tmp!=128){/////////////////////////////////////////128 ����new������Ϣû��
        next_block = new(_virtualDiskAddr + tmp*_blockSize)FCBBLOCK();
        tmp=_fat->fats[tmp];
    }
    return head_block;
}
//根据块号找到block，对于已经初始化过的
FileBLOCK* num2FileBlock(int num) {
    FileBLOCK* B = (FileBLOCK*)(_virtualDiskAddr + num*_blockSize);
    return B;
}
FCBBLOCK* num2FCBBlock(int num) {
    FCBBLOCK* B = (FCBBLOCK*)(_virtualDiskAddr + num*_blockSize);
    return B;
}

//////////////////////////////////////////////////////////////////////////////////////////

//给开发��提供BLOCK，供写入文件时使用；
/*
BLOCK* getEmptyBLOCKNum(int need_BLOCK_Num) {
	if (_BLOCK_count >= need_BLOCK_Num)
	{
		////block记住分配的头
		BLOCK* block = _emptyBLOCK;
		for (int i = 0; i < need_BLOCK_Num - 1; i++)
		{
			_emptyBLOCK = _emptyBLOCK->_nextBlock;
		}
		////temp暂存尾置next空从而从空block
		////empytyblock头移动到新位置，emptyblcokconut减少
		BLOCK* tmp = _emptyBLOCK;
		_emptyBLOCK = _emptyBLOCK->_nextBlock;
		tmp->_nextBlock = NULL;
		_emptyBLOCK_Count -= need_BLOCK_Num;
		return block;
	}
	return NULL;
}*/
int getEmptyBLOCKNum(int need_BLOCK_Num) {//返回可使用空白BLOCK序号，自动记入FAT�?
    if (_BLOCK_count >= need_BLOCK_Num)
    {
        ////block记住分配的头
        int block;
        bool init=false;
        int tmp_block;
        int j = 129;//可优�?
        for (int i = 0; i < need_BLOCK_Num; i++)
        {
            while (_fat->fats[j] != -1 ) j++;
            if (!init) {
                block = j;
                init = true;
                tmp_block = block;
            }
            else {
                _fat->fats[tmp_block] = j;
                tmp_block = j;
            }
            j++;
        }
        _fat->fats[tmp_block] = 128;
        _emptyBLOCK_Count -= need_BLOCK_Num;
        return block;
    }
    return -1;
}

//给开发��提供FCB，供创建文件或目录时使用�?
/*
FCB* getFCB() {
    if (_FCB_count>0)
    {
        //说明有可用的FCB，那么只要返回_emptyFCB中的丢�个，并修改FCB这个链表�?
        FCB* fcb = _emptyFCB;
        _emptyFCB = _emptyFCB->_child;
        fcb->_child = NULL;
        _emptyFCB_Count--;
        return fcb;
    }
    return NULL;         //用户调用文件系统lib时，霢�要判断是否为空，而提供输出信息；
}
*/
FCB* getblankFCB(FCB* parentDir) {///////////////////////////////修改完成，未测试
    //找到destination的目录文件盘块，取其中一个fcb
    int tmp = parentDir->_block;
    FCBBLOCK *destination_fcbblock;
    int beforetmp;
    FCB *fcb;
    if(parentDir->_block!=-1) {
        destination_fcbblock = num2FCBBlock(parentDir->_block);
        while (destination_fcbblock->_contentLength == _fcbsSize) {
            beforetmp = tmp;
            tmp = _fat->fats[tmp];//查找下一个盘
            if (tmp == -1) {//特殊情况:正好扢�有满�?
                //new丢�个新fcbblock，且记录入fat�?
                tmp = getEmptyBLOCKNum(1);
                //destination_fcbblock=num2FCBBlock(tmp);
                destination_fcbblock = initFCBBlock(tmp);
                _fat->fats[beforetmp] = tmp;
                break;
            }
            destination_fcbblock = num2FCBBlock(tmp);
        }
        //目标fcb
        fcb = &(destination_fcbblock->_content[destination_fcbblock->_contentLength]);////&&&&&&&!!!!!!

    }else{
        ///////////////////////////////////////////////////���ǻ�δ���̿�ĵ�Ŀ¼��

        tmp = getEmptyBLOCKNum(1);

        destination_fcbblock = initFCBBlock(tmp);

        parentDir->_block = tmp;

       // _fat->fats[parentDir->_block] = 128;//????????FAT���޸Ĳ���
        fcb = &(destination_fcbblock->_content[destination_fcbblock->_contentLength]);
        ////////////////////////////////////////////////////////////
    }
    destination_fcbblock->_contentLength+=1;///!!!!
    fcb->_access[current_usernum]='4';////�����������Ȩ��
    return fcb;
}


/*
//删除文件时，回收并清理BLOCK�?
void releaseBLOCK(BLOCK* block) {
	if (block == NULL)
		return;
	_lastBLOCK->_nextBlock = block;
	_lastBLOCK = _lastBLOCK->_nextBlock;
	strcpy(_lastBLOCK->_content, "");
	while (_lastBLOCK != NULL)
	{
		_lastBLOCK->_contentLength = 0;
		_emptyBLOCK_Count++;
		_lastBLOCK = _lastBLOCK->_nextBlock;
	}
}
*/
//删除文件时，回收并清理BLOCK�?////////////////////////修改完成，未测试
void releaseBLOCK(int block,int type) {//释放内存空间，修改fat�?
    //释放首块
    if (block == -1)
        return;
    if(type==1){
        FCBBLOCK* rmfcbblock=num2FCBBlock(block);
        int num=0;
        while(num<rmfcbblock->_contentLength) {
            releaseBLOCK(rmfcbblock->_content[num]._block, rmfcbblock->_content[num]._type);
            num++;
        }
        delete rmfcbblock;
    }
    else{
        FileBLOCK* rmfileblock=num2FileBlock(block);
        delete rmfileblock;
    }
    _emptyBLOCK_Count++;
    //释放后面的块
    int next_block=_fat->fats[block];
    while (next_block != -1) {
        next_block = _fat->fats[block];
        _fat->fats[block] = -1;

        if(type==1){
            FCBBLOCK* rmfcbblock=num2FCBBlock(block);
            int num=0;
            while(num<rmfcbblock->_contentLength) {
                releaseBLOCK(rmfcbblock->_content[num]._block, rmfcbblock->_content[num]._type);
                num++;
            }
            delete rmfcbblock;
        }
        else{
            FileBLOCK* rmfileblock=num2FileBlock(block);
            delete rmfileblock;
        }
        _emptyBLOCK_Count++;
        block = next_block;
    }
}

//删除文件/目录时，回收并清理FCB；������������删除带来大麻烦,对应添加也要�?//修改完成，未测试
void releaseFCB(FCBBLOCK* block,int fcbnum) {
    releaseBLOCK(block->_content[fcbnum]._block,block->_content[fcbnum]._type);
    //修改本block
    while(fcbnum+1<(block->_contentLength)){
        block->_content[fcbnum]=block->_content[fcbnum+1];
        fcbnum++;
    }
    FCB* fcb=&(block->_content[fcbnum]);//&!!!
    strcpy(fcb->_name, "");
    fcb->_type = 1;
    fcb->_size = 0;
    fcb->_block = -1;
    //_emptyFCB_Count++;
    block->_contentLength--;
}

//删除目录和文件中有个相同的操作就是回收FCB，现抽取出来�?/////////////////////修改完成，未测试
//删除目录和文s件从回收FCB弢��?/////////////////////////////////////////////////修改完成，未测试
void delete_dirOrFile(FCBBLOCK* block,int fcbnum) {
    releaseFCB(block,fcbnum);
    //回收FCB�?
    /*
   if (deleting->_type == 1)
   {
       //删除的是目录�?
       //先删除这个目录中的内容；
       //删除这个目录中时，需要先删除child////brother�?
       while (deleting->_child != NULL)
       {
           delete_dirOrFile(deleting->_child);
       }
       recoverFCB(deleting);
   }
   else
   {
       //是文件；直接回收就好�?
       recoverFCB(deleting);
   }*/
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////修改完成，未测试
//在当前目录下搜索FCB�?
FCB* returnSonFCB(FCB* currentDir, string name,int type,int &B,int &F) {
    int fcbnum;
    int tmp_blocknum=currentDir->_block;
    FCBBLOCK* tmp_fcbblock;
    FCB* fcb;

    while(tmp_blocknum!=-1){

        tmp_fcbblock=num2FCBBlock(tmp_blocknum);
        fcbnum=0;
        while (fcbnum<(tmp_fcbblock->_contentLength))
        {
            fcb=&(tmp_fcbblock->_content[fcbnum]);
            if (fcb->_name == name && fcb->_type == type)
            {

                B=tmp_blocknum;
                F=fcbnum;
                //说明找到了；
                return fcb;
            }
            fcbnum++;
        }
        tmp_blocknum=_fat->fats[tmp_blocknum];
    }
    return NULL;           //由开发��据此提供错误信息；
}
/*
//返回parent，在给定的目录中，判断目录是否存在，不存在返回NULL，存在返回这个目录FCB�?
FCB* return_DIR_FCB(string DIR[], int count, bool isAbsolutePath)
{
    FCB* tmp;
    if (isAbsolutePath)
    {
        tmp = _root;
    }
    else
    {
        tmp = _current;
    }

    for (int i = 0; i < count; i++)
    {
        tmp = returnSonFCB(tmp, DIR[i], 1);
        if (tmp == NULL)
        {
            return NULL;           //说明当前目录下没有这个子目录�?
        }
    }
    return tmp;
}*/
FCB* return_FCB(string DIR[], int count, int type,FCB* &parent,int &B,int &F,bool isAbsolutePath){
        FCB* currentDir;
        currentDir=_current;//_root
        if(isAbsolutePath){
            int tmp_c=0;
            while(tmp_c<count-1) {
                currentDir=returnSonFCB(currentDir,DIR[tmp_c],1,B,F);
                if(currentDir==NULL) return NULL;
                tmp_c++;
            }
        }
        else currentDir=_current;
        parent =currentDir;//父目录存�?
        return returnSonFCB(currentDir,DIR[count-1],type,B,F);
}


////可以实现路径---对目录和文件的访问；//此处有疑问？？？？？？？？？
/////总的概括的，层级调用前面�?
FCB* sys_returnFCB(string path_name, string &name, int type,FCB* &parent,int &B,int &F) {//支持绝对路径和相对路�?--找到扢�霢�FCB
        string* DIR = new string[DirDepth]; //string DIR[DirDepth];
        int i = 0;
        ////拆分路径�?
        for (int j = 0; j < path_name.length(); j++) {
            if (path_name[j] == '/') {
                if (DIR[i] != "")
                    i++;
                else continue;
            }
            else {
                DIR[i] += path_name[j];
            }
        }
        ////统计路径段个�?
        int count = 0;
        for (i = 0; i < 20; i++) {
            if (DIR[i] != "") {
                count++;
            }
            else {
                break;
            }
        }
        if (count == 0&&path_name[0]=='/')//两个条件同时满足为root
        {
            //说明用户的路�?
            name = "root";
            return _root;
        }
        else
        {
            name = DIR[count - 1];      //这个为路径中的最后一个字段；
        }

        bool isAbsolutePath=false;
        if (path_name[0] == '/') isAbsolutePath=true;

        return return_FCB(DIR, count,type,parent,B,F,isAbsolutePath);

/*
    if (count == 0&&path_name[0]=='/')//两个条件同时满足为root
    {
        //说明用户的路�?
        name = "root";
        parent = _root;
        return _root;
    }
    else
    {
        name = DIR[count - 1];      //这个为路径中的最后一个字段；
    }

    bool AbsolutePath = false;
    if (path_name[0] == '/') {
        AbsolutePath = true;
    }
    parent = return_DIR_FCB(DIR, count - 1, AbsolutePath);
    if (parent != NULL)
    {

        return returnSonFCB(parent, name, type);
    }
    else
    {
        return NULL;
    }
	*/
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



//求要复制的，霢�要多少系统资源，FCB个数，BLOCK个数//////////////////////////原代码已被修�?完成，未测试
int num_dirOrFile(FCB* adding) {

    //numFCB++;
    int numBLOCK=0;
    int type = adding->_type;
    if (type == 0)//说明adding是个文件�?
    {
        //只需要计算adding的BLOCK�?
/*�����������bug
        double needAddBLOCK;
        needAddBLOCK = adding->_size / _contentSize;
        int tmp = needAddBLOCK;
*/
        int needAddBLOCK;
        needAddBLOCK = adding->_size / _contentSize;
        int tmp = adding->_size % _contentSize;
        if (tmp!=0)
        {
            //说明没损失，扢�以分配的个数为tmp;
            numBLOCK += 1;
        }
    }
    else//说明adding是个目录�?
    {//找到adding下的扢�有内�?
        //FCB* tmp=adding->child;
        FCBBLOCK* tmpblock=num2FCBBlock(adding->_block);
        int i=0;
        while (i<tmpblock->_contentLength)
        {
            //递归计算目录下内容；
            numBLOCK+=num_dirOrFile(&(tmpblock->_content[i]));
            //tmp = tmp->_brother;
            i++;
        }
        int tmp=_fat->fats[adding->_block];
        while(tmp!=-1){
            tmpblock=num2FCBBlock(tmp);
            i=0;
            while (i<tmpblock->_contentLength)
            {
                //递归计算目录下内容；
                numBLOCK+=num_dirOrFile(&(tmpblock->_content[i]));
                i++;
            }
            tmp=_fat->fats[tmp];
        }
    }
    return numBLOCK;
}

//复制,当是目录时，霢�要copy目录里的扢�有内容；/////////////////////////////// /原代码已被修�?修改完成，未测试
/*
void add_dirOrFile(FCB* adding, FCB* destination) {

    //destination ，复制到的那个目录；adding，需要复制的内容�?
    //因为要增加这个文�?目录，所以都霢�要把这个节点复制丢�份移入目录树中；
    FCB* copy_adding = getFCB();
    strcpy(copy_adding->_name, adding->_name);
    //copy_adding->_spare = adding->_spare;
    copy_adding->_type = adding->_type;
    copy_adding->_size = adding->_size;
    copy_adding->_block = adding->_block;
    //这是区分目录和文件的关键�?
    copy_adding->_child = NULL;
    ////下面两行实现了移入目录树中；
    copy_adding->_parent = destination;
    copy_adding->_brother = destination->_child;
    destination->_child = copy_adding;

    //这里必须断掉brother;
    //copy_adding->_child->_brother = NULL;


    FCB* tmp;//保存移动目标的子节点---文件or下一�?
    tmp = adding->_child;

    if (tmp == NULL)
    {
        //说明adding是个文件�?
        //则需要add声请BLOCK，存放内容；

        //该文件有内容，需要增添BLOCK�?

        //霢�要添加几个；
        double needAddBLOCK;

        needAddBLOCK = copy_adding->_size / _contentSize;
        if (needAddBLOCK==0)
        {
            //不需要BLOCK�?
        }
        else
        {
            int tmp = needAddBLOCK;

            ////BLOCK* contentFirstAdrr;   //分配的BLOCK的首地址�?
            int contentFirstBlockNum;

            if (needAddBLOCK == tmp)
            {
                //说明没损失，扢�以分配的个数为tmp;
                ////contentFirstAdrr = getEmptyBLOCKNum(tmp);
                contentFirstBlockNum = getEmptyBLOCKNum(tmp);

            }
            else
            {
                //精度损失；上取整�?
                ////contentFirstAdrr = getEmptyBLOCKNum(tmp + 1);
                contentFirstBlockNum = getEmptyBLOCKNum(tmp);
            }

            ////copy_adding->_block = contentFirstAdrr;
            copy_adding->_block = contentFirstBlockNum;

            //指向adding的BLOCK�?
            ////BLOCK* tmp_addingBLOCK;
            ////tmp_addingBLOCK = adding->_block;
            int tmp_addingBLOCKNum = adding->_block;
            BLOCK* contentFirstAdrr = num2Block(contentFirstBlockNum);
            BLOCK* tmp_addingBLOCK = num2Block(tmp_addingBLOCKNum);
            //先处理整数内容的BLOCK�?
            for (int i = 0; i < tmp - 1; i++)
            {
                ////strcpy(contentFirstAdrr->_content, ((string)(tmp_addingBLOCK->_content)).substr(i*_contentSize, _contentSize).c_str());
                strcpy(contentFirstAdrr->_content, ((string)(tmp_addingBLOCK->_content)).substr(0, _contentSize).c_str());
                contentFirstAdrr->_contentLength = _contentSize;
                contentFirstAdrr = num2Block(_fat->fats[contentFirstBlockNum]);
                contentFirstBlockNum = _fat->fats[contentFirstBlockNum];//-1??????
                tmp_addingBLOCK = num2Block(_fat->fats[contentFirstBlockNum]);
                tmp_addingBLOCKNum = _fat->fats[tmp_addingBLOCKNum];//-1??????
            }
            //存取朢�后一个剩余内容；
            contentFirstAdrr->_contentLength = tmp_addingBLOCK->_contentLength;
            strcpy(contentFirstAdrr->_content, ((string)(tmp_addingBLOCK->_content)).substr(0, contentFirstAdrr->_contentLength).c_str());
            /*
            //先处理整数内容的BLOCK�?
            for (int i = 0; i < tmp - 1; i++)
            {//////？？？？？？？？？？？？？？？？�?
                strcpy(contentFirstAdrr->_content, ((string)(tmp_addingBLOCK->_content)).substr(i*_contentSize, _contentSize).c_str());
                contentFirstAdrr->_contentLength = _contentSize;
                contentFirstAdrr = contentFirstAdrr->_nextBlock;
                tmp_addingBLOCK = tmp_addingBLOCK->_nextBlock;
            }
            //存取朢�后一个剩余内容；
            contentFirstAdrr->_contentLength = tmp_addingBLOCK->_contentLength;
            strcpy(contentFirstAdrr->_content, ((string)(tmp_addingBLOCK->_content)).substr(0, contentFirstAdrr->_contentLength).c_str());
            //
        }
    }
    else
    {
        //先拷贝child;
        add_dirOrFile(tmp, copy_adding);
        tmp = tmp->_brother;
        while (tmp != NULL)
        {
            //本质是拷贝文件，扢�以文�?目录实现方式相同；��归调用--注意递归出口也即统一�?
            add_dirOrFile(tmp, copy_adding);
            tmp = tmp->_brother;
        }
    }

}*/

void add_dirOrFile(FCB* adding, FCB* destination) {//destination ，复制到的那个目录；adding，需要复制的内容�?/单纯执行复制操作，不用��虑重名等问�?
/*
	//找到destination的目录文件盘块，取其中一个fcb
	FCBBLOCK* destination_fcbblock=num2FCBBlock(destination->_block);
	int tmp=destination->_block;
	int beforetmp;
	while(destination_fcbblock->_contentLength==_fcbsSize){
		beforetmp=tmp;
		tmp=_fat->fats[tmp];//查找下一个盘
		if(tmp==-1){//特殊情况:正好扢�有满�?
			//new丢�个新fcbblock，且记录入fat�?
			tmp=getEmptyBLOCKNum(1);
			//destination_fcbblock=num2FCBBlock(tmp);
			destination_fcbblock=initFCBBlock(tmp);
			_fat->fats[beforetmp]=tmp;
			break;
		}
		destination_fcbblock=num2FCBBlock(tmp);
	}
	//设置目标fcb
	FCB* destination_fcb=destination_fcbblock->_content[destination_fcbblock->_contentLength];*/
    FCB* destination_fcb=getblankFCB(destination);
    //根据addingfcb设置目标fcb基本信息
    strcpy(destination_fcb->_name, adding->_name);
    destination_fcb->_type = adding->_type;
    destination_fcb->_size = adding->_size;
    ////copy_adding->_block = adding->_block;



    int needAddBLOCK=num_dirOrFile(adding);
    int contentFirstBlockNum = getEmptyBLOCKNum(needAddBLOCK);

    //设置FCB数据结构中物理存储BLOCK//目录是FCBBLOCK，文件是FileBLOCK
    int type = adding->_type;
    if (type == 0)
    {
        //说明adding是个文件�?
        //则需要add声请BLOCK，存放内容；

        //该文件有内容，需要增添BLOCK�?
        //霢�要添加几个；
        /*
        double needAddBLOCK;
        needAddBLOCK = adding->_size / _contentSize;
        if (needAddBLOCK==0)
        {
            //不需要BLOCK�?
        }
        else
        {
            int tmp = needAddBLOCK;

            ////BLOCK* contentFirstAdrr;   //分配的BLOCK的首地址�?
            int contentFirstBlockNum;

            if (needAddBLOCK == tmp)
            {
                //说明没损失，扢�以分配的个数为tmp;
                ////contentFirstAdrr = getEmptyBLOCKNum(tmp);
                contentFirstBlockNum = getEmptyBLOCKNum(tmp);

            }
            else
            {
                //精度损失；上取整�?
                ////contentFirstAdrr = getEmptyBLOCKNum(tmp + 1);
                contentFirstBlockNum = getEmptyBLOCKNum(tmp+1);
            }
            */
        destination_fcb->_block = contentFirstBlockNum;

        FileBLOCK* contentFirstAdrr =initFileBlock(contentFirstBlockNum);//num2FileBlock(contentFirstBlockNum);

        //指向adding的BLOCK�?
        ////BLOCK* tmp_addingBLOCK;
        ////tmp_addingBLOCK = adding->_block;
        int tmp_addingBLOCKNum = adding->_block;
        FileBLOCK* tmp_addingBLOCK = num2FileBlock(tmp_addingBLOCKNum);
        //先处理整数内容的BLOCK�?
        for (int i = 0; i < needAddBLOCK - 1; i++)
        {
            ////strcpy(contentFirstAdrr->_content, ((string)(tmp_addingBLOCK->_content)).substr(i*_contentSize, _contentSize).c_str());
            strcpy(contentFirstAdrr->_content, ((string)(tmp_addingBLOCK->_content)).substr(0, _contentSize).c_str());
            contentFirstAdrr->_contentLength = _contentSize;
            contentFirstAdrr = num2FileBlock(_fat->fats[contentFirstBlockNum]);
            contentFirstBlockNum = _fat->fats[contentFirstBlockNum];//-1??????
            tmp_addingBLOCK = num2FileBlock(_fat->fats[contentFirstBlockNum]);
            tmp_addingBLOCKNum = _fat->fats[tmp_addingBLOCKNum];//-1??????
        }
        //存取朢�后一个剩余内容；
        contentFirstAdrr->_contentLength = tmp_addingBLOCK->_contentLength;
        strcpy(contentFirstAdrr->_content, ((string)(tmp_addingBLOCK->_content)).substr(0, contentFirstAdrr->_contentLength).c_str());
    }

    else//adding是目录文�?
    {

        //double needAddBLOCK;
        //needAddBLOCK = adding->_size / _fcbsSize;
        if(needAddBLOCK==0){
        }else{
            int contentFirstBlockNum;
            contentFirstBlockNum = getEmptyBLOCKNum(needAddBLOCK);
            destination->_block = contentFirstBlockNum;
            FCBBLOCK* contentFirstAdrr =initFCBBlock(contentFirstBlockNum);

            int childfcb_num;
            int childblock_num=contentFirstBlockNum;
            FCBBLOCK* childFCBBlock;
            FCB* childfcb;
            while(childblock_num!=-1){
                childFCBBlock=num2FCBBlock(childblock_num);
                childfcb_num=0;
                while(childfcb_num<childFCBBlock->_contentLength){
                    childfcb=&(childFCBBlock->_content[childfcb_num]);
                    add_dirOrFile(childfcb, destination_fcb);
                    childfcb_num++;
                }
                childblock_num=_fat->fats[childblock_num];
            }
        }

    }

}

int share_dirOrFile(FCB* shared, FCB* sharing) {
    if(shared->_access[current_usernum]!='0') {
        sharing->_block = shared->_block;
        return 1;
    }else return 0;
}

/**********************************************************************/
//目录管理声明�?
/**********************************************************************/
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int sys_createuser(string username){
    if(_users->_contentLength<_fcbsSize){
        int u=0;
        while(u<_users->_contentLength){//用户不存�?
            if(_users->_content[u]._name==username) ///???????语法是否正确
                return -2;//用户已存�?
            u++;
        }
        FCB* user=&(_users->_content[u]);
        strcpy(user->_name, username.c_str());
        user->_access[u]='4';
        _users->_contentLength+=1;

        return 1;//用户添加成功
    }
    else return -1;//添加用户失败，用户达上限
}

int sys_suroot(){
    _current=_root;
    current_path="//";
    current_usernum=0;
    current_user=_root;
    return 1;//用户已存�?

}
int sys_su(string name){
    int u=0;
    while(u<_users->_contentLength){//用户不存�?
        if(_users->_content[u]._name==name){ ///???????语法是否正确
            _current=&(_users->_content[u]);
            current_path="//";
            current_usernum=u;
            current_user=&(_users->_content[u]);
            return 1;//用户已存�?
        }
        u++;
    }
    return -1;//用户添加成功
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//查看当前目录内容�?/stringlist返回打印内容(正确内容or目录不存�?
//查看当前目录内容；扩展：实现查看任意路径下的内容�?///////////////////修改完成，未测试
StringList* sys_dir(string dirPath) {

    string dirName = "";             //用户想要查看的目录的名字；在下面函数获得�?
    const int dir_type = 1;            //丢�定是目录�?
    //霢�要知道用户想查看的目录存不存在；
    FCB*  parentDir = NULL;      //parentDir父目录，目前是不知道的，扢�以设置为NULL，��过下述过程，可以知道parentDir是否存在，以及想查看的目录是否存在；
    int B,F;
    StringList *dir_content = new StringList();//用StringList 存取霢�要打印的信息�?
    StringList *tmp_dirContent;
    FCB* tmp;

    if (dirPath != ""){
        ////////////////////////////////////////////////////
        if (dirPath[0] != '\\') {
            dir_content->content = "·����ʽ����\n";
            dir_content->next = NULL;
            return dir_content;
        }
        ////////////////////////////////////////////////////
        tmp = sys_returnFCB(dirPath, dirName, dir_type, parentDir,B,F);       //获得要查看的目录�?
    }
    else tmp = _current;

    if (tmp != NULL) {//要查看的目录存在�?
        if (tmp->_access[current_usernum] == '0') {
            dir_content->content += "��ǰĿ¼����Ȩ����\n";
            dir_content->next = NULL;
            return dir_content;
        }else {
            tmp_dirContent = dir_content;//tmp_dirContent，dir_content的临时副本，由于是指针内容写入的是同丢�地址
            if (tmp->_block == -1) {
                dir_content->content += "��ǰĿ¼Ϊ��\n";
                dir_content->next = NULL;
                return dir_content;
            } else {
                int fcbblocknum = tmp->_block;
                FCBBLOCK *fcbblock = num2FCBBlock(fcbblocknum);
                if (fcbblock->_contentLength == 0) {
                    dir_content->content += "��ǰĿ¼Ϊ��\n";
                    dir_content->next = NULL;
                    return dir_content;
                }
                int fcbnum;
                FCB *fcb;
                while (fcbblocknum != -1&&fcbblocknum != 128) {
                    fcbnum = 0;
                    fcbblock = num2FCBBlock(fcbblocknum);
                    while (fcbnum < fcbblock->_contentLength) {
                        fcb = &(fcbblock->_content[fcbnum]);
                        if (fcb->_type == 1) {
                            //cout << "Dir:	" << tmp->name << endl;
                            tmp_dirContent->content +=
                                    "Dir:" + (string) fcb->_name + " size:" + to_string(fcb->_size) + "\n";
                            tmp_dirContent->next = new StringList();
                            tmp_dirContent = tmp_dirContent->next;
                        } else {
                            tmp_dirContent->content +=
                                    "File:" + (string) fcb->_name + " size:" + to_string(fcb->_size) + "\n";
                            tmp_dirContent->next = new StringList();
                            tmp_dirContent = tmp_dirContent->next;
                        }
                        fcbnum++;
                    }
                    fcbblocknum = _fat->fats[fcbblocknum];
                }
            }
            return dir_content;
        }
    }
    else
    {
        if (parentDir==NULL)
        {
            dir_content->content = "��Ŀ¼�ĸ�Ŀ¼�����ڣ�\n";
            dir_content->next = NULL;
            return dir_content;
        }
        else
        {
            //说明想要删除的目录不存在�?
            dir_content->content = "��Ŀ¼�����ڣ�\n";
            dir_content->next = NULL;
            return dir_content;
        }
    }
}

//创建目录�?//////////////////////////////////////////////////(fat让代码更箢�单了)修改完成，未测试
int sys_mkdir(string dirPath) {

    const int dir_type = 1;

    //霢�要知道用户想创建的目录存不存在；
    FCB*  parentDir = NULL;      //parentDir父目录，目前是不知道的，扢�以设置为NULL，��过下述过程，可以知道parentDir是否存在，以及想创建的目录是否存在；
    string dirName = "";             //用户想要创建的目录的名字；在下面函数获得�?
    int B,F;
    if (sys_returnFCB(dirPath, dirName, dir_type, parentDir,B,F) == NULL) {
        //说明想要创建的目录不存在，继续判断想在哪个目录创建，那个目录是否存在

        if (parentDir!=NULL)
        {
            if (parentDir->_access[current_usernum] == '3' || parentDir->_access[current_usernum] == '4') {

                FCB *dirfcb = getblankFCB(parentDir);
                strcpy(dirfcb->_name, dirName.c_str());
                //目录创建成功；返�?；提示开发��，向用户显示成功信息；
                return 1;
            }else return 0;
        }
        else
        {
            //提示弢�发��，由开发��告知用户，想要创建的那个目录所在的目录不存在，应该先创建它的父目录�?
            return -1;
        }
    }
    else
    {
        //提示弢�发��，由开发��告知用户，想要创建的目录已存在，不允许创建�?
        return -2;
    }
}

//删除目录;/////////////////////////////////////////////////////////////修改完成，未测试
int sys_rmdir(string dirPath) {
    const int dir_type = 1;

    //霢�要知道用户想删除的目录存不存在；
    FCB*  parentDir = NULL;      //parentDir父目录，目前是不知道的，扢�以设置为NULL，��过下述过程，可以知道parentDir是否存在，以及想删除的目录是否存在；
    string dirName = "";             //用户想要删除的目录的名字；在下面函数获得�?
    int B,F;
    FCB* tmp_deleteDir; //指向要删除的目录�?
    tmp_deleteDir = sys_returnFCB(dirPath, dirName, dir_type, parentDir,B,F);

    if (tmp_deleteDir != NULL) {
        //说明想要删除的目录存在；
        if (tmp_deleteDir->_access[current_usernum] == '3' || tmp_deleteDir->_access[current_usernum] == '4') {

            delete_dirOrFile(num2FCBBlock(B), F);
            return 1;                          //提示弢�发��，由开发��告知用户，删除目录成功�?
        }else return 0;
    }
    else
    {
        if (parentDir == NULL)
        {
            return -1;          //提示弢�发��，想要删除的目录的父目录不存在�?
        }
        else
        {
            //说明想要删除的目录不存在�?
            return -2;               //提示弢�发��，由开发��告知用户，删除的目录不存在�?
        }
    }

}
/*
*文件操作
*/
//创建文件�?////////////////////////////////////////////////////////////修改完成，未测试
int sys_create_file(string filePath) {
    const int file_type = 0;//防止误改

    //霢�要知道用户想创建的文件存不存在；
    FCB*  parentDir = NULL;      //parentDir父目录，目前是不知道的，扢�以设置为NULL，��过下述过程，可以知道parentDir是否存在，以及想创建的文件是否存在；
    string fileName = "";             //用户想要创建的文件的名字；在下面函数获得�?
    int B,F;
    if (sys_returnFCB(filePath, fileName, file_type, parentDir,B,F) == NULL) {
        //说明想要创建的文件不存在，继续判断想在哪个目录创建，那个目录是否存在
        if (parentDir != NULL)
        {
            if (parentDir->_access[current_usernum] == '3' || parentDir->_access[current_usernum] == '4') {
                //说明想在那个目录创建是合法的�?
                //内核分配FCB�?
                FCB *fcboffile;
                fcboffile = getblankFCB(parentDir);     //返回给开发��一个FCB；待弢�发��使用；
                strcpy(fcboffile->_name, fileName.c_str());
                fcboffile->_type = 0;
                fcboffile->_block = -1;
                fcboffile->_size = 0;
                //文件创建成功；返�?；提示开发��，向用户显示成功信息；
                return 1;
            }else return 0;

        }
        else
        {
            //提示弢�发��，由开发��告知用户，想要创建的那个文件所在的目录不存在，应该先创建它的父目录�?
            return -1;
        }
    }
    else
    {
        //提示弢�发��，由开发��告知用户，想要创建的文件已存在，不允许创建�?
        return -2;
    }
}

//删除文件�?///////////////////////////////////////////////////////////修改完成，未测试
int sys_delete_file(string filePath) {

    const int file_type = 0;

    //霢�要知道用户想删除的文件存不存在；
    FCB*  parentDir = NULL;      //parentDir父目录，目前是不知道的，扢�以设置为NULL，��过下述过程，可以知道parentDir是否存在，以及想删除的文件是否存在；
    string fileName = "";             //用户想要删除的文件的名字；在下面函数获得�?
    int B,F;
    FCB* tmp_deleteFile; //指向要删除的文件控制块；
    tmp_deleteFile = sys_returnFCB(filePath, fileName, file_type, parentDir,B,F);

    if (tmp_deleteFile != NULL) {
        if (tmp_deleteFile->_access[current_usernum] == '3' || tmp_deleteFile->_access[current_usernum] == '4') {
            //说明想要删除的文件控制块存在
            delete_dirOrFile(num2FCBBlock(B), F);
            return 1;                          //提示弢�发��，由开发��告知用户，删除文件成功�?
        }else return 0;
    }
    else
    {
        //说明想要删除的文件不存在�?
        return -1;               //提示弢�发��，由开发��告知用户，删除的文件不存在�?
    }
}

//重命名；支持文件和目�?///////////////////////////////////////////////修改完成，未测试(函数封装好了后修改就很方�?
int sys_rename(string path, int type, string new_name) {
    //先判读用户给定的路径是否合法；即能否找到这个文件/目录�?
    //type,有开发��询问用户后，获得；

    FCB*  parentDir = NULL;      //parentDir父目录，目前是不知道的，扢�以设置为NULL，��过下述过程，可以知道parentDir是否存在，以及想重命名的文件是否存在�?
    string oldName = "";             //用户想要重命名的文件的名字；在下面函数获得；
    int B,F;
    FCB* rename_FCB = sys_returnFCB(path, oldName, type, parentDir,B,F);

    if (rename_FCB != NULL) {
        //想重命名的文�?目录存在�?
        if (rename_FCB->_access[current_usernum] == '0' || rename_FCB->_access[current_usernum] == '1') return 0;
        else {
            //判断新名字是否在父目录中重复�?
            if (returnSonFCB(parentDir, new_name, type, F, B) == NULL) {
                //说明，新名字没有重复�?
                strcpy(rename_FCB->_name, new_name.c_str());
                return 1;        //提示弢�发��重命名成功�?
            } else {
                //同名同类型已经存在，由于是遍历父目录下所有的孩子，可能是和自己重复，但拒绝重命名效果丢�样；
                return -1;              //提示弢�发��，存在同名同类型；
            }
        }
    }
    else
    {
        //想重命名的文件不存在�?
        if (parentDir == NULL)
        {
            //由于父目录不存在�?
            return -2;       //提示弢�发��，父目录不存在�?

        }
        else
        {
            return -3;         //提示弢�发��，想重命名的文件不存在�?
        }
    }
}
//向文件覆盖写内容�?/////////////////////////////////////////////////////修改完成，未测试
int sys_overwrite_file(FCB* file, string content) {
    //文件是否存在，此时内核不考虑，由弢�发��处理；
    //扢�以，此时文件已经存在�?
    if (file->_access[current_usernum] == '0' || file->_access[current_usernum] == '1') return 0;
    else {
        //根据content分配block个数�?
        double needUsedBlock;
        int tmp;
        needUsedBlock = content.length() / _contentSize;
        tmp = content.length() % _contentSize;


        //BLOCK* contentFirstAdrr;   //分配的BLOCK的首地址�?
        int contentFirstBlockNum;   //分配的BLOCK的首块号�?

        if (!tmp) {
            //说明没损失，扢�以分配的个数为tmp;
            contentFirstBlockNum = getEmptyBLOCKNum(needUsedBlock);


            ////if (contentFirstAdrr == NULL)
            if (contentFirstBlockNum == -1) {
                return -1;              //提示弢�发��，扢�霢�要的BLOCK不够，写文件失败�?
            }

        } else {

            //精度损失；上取整�?
            needUsedBlock++;
            contentFirstBlockNum = getEmptyBLOCKNum(needUsedBlock);

            if (contentFirstBlockNum == -1) {
                return -1;              //提示弢�发��，扢�霢�要的BLOCK不够，写文件失败�?
            }
        }

        if (file->_block != -1) {
            releaseBLOCK(file->_block, file->_type);
        }
        file->_block = contentFirstBlockNum;
        file->_size = content.length();

        FileBLOCK *contentFirstAdrr = initFileBlock(contentFirstBlockNum);
        //先处理整数内容的BLOCK�?
        for (int i = 0; i < needUsedBlock - 1; i++) {
            /////strcpy(contentFirstAdrr->_content, content.substr(i*_contentSize, _contentSize).c_str());
            strcpy(contentFirstAdrr->_content, content.substr(i * _contentSize, _contentSize).c_str());
            //content.substr(i*_contentSize, _contentSize).copy(contentFirstAdrr->_contentStart, content.substr(i*_contentSize, _contentSize).length());
            contentFirstAdrr->_contentLength = _contentSize;

            /////contentFirstAdrr = contentFirstAdrr->_nextBlock;
            contentFirstAdrr = num2FileBlock(_fat->fats[contentFirstBlockNum]);
            contentFirstBlockNum = _fat->fats[contentFirstBlockNum];//-1??????
        }
        //存取剩余内容�?
        contentFirstAdrr->_contentLength = tmp;
        strcpy(contentFirstAdrr->_content, content.substr((needUsedBlock - 1) * _contentSize, tmp).c_str());
        //content.substr((tmp - 1)*_contentSize, contentFirstAdrr->_contentLength).copy(contentFirstAdrr->_contentStart, content.substr((tmp - 1)*_contentSize, contentFirstAdrr->_contentLength).length());
        return 1;       //提示弢�发��，写文件成功；

    }
}
//向文件追加写内容�?///////////////////////////////////////////////////修改完成，未测试
/*
int sys_appendwrite_file(FCB* file, string content) {
    //文件是否存在，此时内核不考虑，由弢�发��处理；
    //扢�以，此时文件已经存在�?
    if (file->_access[current_usernum] == '0' || file->_access[current_usernum] == '1') return 0;
        //根据content分配block个数�?
    else {
        double needUsedBlock, UsedBlock;
        int tmp1, tmp0;
        needUsedBlock = (content.length() + file->_size) / _contentSize;
        tmp1 = (content.length() + file->_size) % _contentSize;
        UsedBlock = file->_size / _contentSize;
        tmp0 = file->_size % _contentSize;
        if (tmp1 != 0) needUsedBlock++;
        if (tmp0 != 0) UsedBlock++;
        int newcontentFirstBlockNum;   //新分配的BLOCK的首块号�?
        int tmp = file->_block;;//原文件末块号
        int newUsedBlock = needUsedBlock - UsedBlock;//新分配的盘块�?
        FileBLOCK *contentFirstAdrr;

        FileBLOCK *contentlastAdrr = num2FileBlock(tmp);

        file->_size += content.length();

        if (needUsedBlock > UsedBlock) {
            newcontentFirstBlockNum = getEmptyBLOCKNum(newUsedBlock);
            if (newcontentFirstBlockNum == -1) {
                return -1;              //提示弢�发��，扢�霢�要的BLOCK不够，写文件失败�?
            }
            ////if (file->_block != NULL)
            if (file->_block == -1)
                file->_block = newcontentFirstBlockNum;
            else {
                //tmp=file->_block;//feel
                //while(tmp!=-1)tmp=_fat->fats[tmp];//tmp=-1
                while (_fat->fats[tmp] != -1)tmp = _fat->fats[tmp];//_fat->fats[tmp]=-1
                _fat->fats[tmp] = newcontentFirstBlockNum;
            }
            contentFirstAdrr = initFileBlock(newcontentFirstBlockNum);
        }else{
            contentFirstAdrr=contentlastAdrr;
        }
        //原末尾块语法正确�????????????????
        string tmpstr = contentlastAdrr->_content;
        tmpstr += content.substr(0, _contentSize - tmp0);
        strcpy(contentlastAdrr->_content, tmpstr.c_str());
        //先处理整数内容的BLOCK�?

        for (int i = 0; i < newUsedBlock - 1; i++) {
            /////strcpy(contentFirstAdrr->_content, content.substr(i*_contentSize, _contentSize).c_str());
            strcpy(contentFirstAdrr->_content,
                   content.substr(_contentSize - tmp0 + i * _contentSize, _contentSize).c_str());
            //content.substr(i*_contentSize, _contentSize).copy(contentFirstAdrr->_contentStart, content.substr(i*_contentSize, _contentSize).length());
            contentFirstAdrr->_contentLength = _contentSize;

            /////contentFirstAdrr = contentFirstAdrr->_nextBlock;
            contentFirstAdrr = num2FileBlock(_fat->fats[newcontentFirstBlockNum]);
            newcontentFirstBlockNum = _fat->fats[newcontentFirstBlockNum];//-1??????
        }
        //存取剩余内容�?
        contentFirstAdrr->_contentLength = tmp1;
        strcpy(contentFirstAdrr->_content,content.substr(_contentSize - tmp0 + (newUsedBlock - 1) * _contentSize, tmp1).c_str());
        //content.substr((tmp - 1)*_contentSize, contentFirstAdrr->_contentLength).copy(contentFirstAdrr->_contentStart, content.substr((tmp - 1)*_contentSize, contentFirstAdrr->_contentLength).length());
        return 1;       //提示弢�发��，写文件成功；
    }
}*/

//读取文件内容�?
string sys_read_file(FCB* file,bool &flag) {
    //文件是否存在，此时内核不考虑，由弢�发��处理；
    //扢�以，此时文件已经存在�?
    flag=true;
    if (file->_access[current_usernum] == '0'){
        flag=false;
        return "";//NULL;
    }
    else {
        string content = "";
        ////BLOCK* tmp = file->_block;
        int tmpnum = file->_block;


        FileBLOCK *tmp = num2FileBlock(tmpnum);
        /*
        while (tmp->_nextBlock != NULL)
        {
        content += tmp->_content;
        tmp = tmp->_nextBlock;
        }
        */
        while (_fat->fats[tmpnum] != -1) {
            content += tmp->_content;
            tmpnum = _fat->fats[tmpnum];
            tmp = num2FileBlock(tmpnum);
        }
        content += ((string) tmp->_content).substr(0, tmp->_contentLength);

        content += "\n read success!";
        return content;
    }
}

//进入子目录；
int sys_cd(string path, string &name)
{
    const int dir_type = 1;

    FCB* parent = NULL;         //子目录的父目录；
    FCB* subDir;        //待查找的子目录；
    int B,F;

    subDir = sys_returnFCB(path, name, dir_type, parent,B,F);



    if (subDir != NULL)
    {
        if (subDir->_access[current_usernum] == '0') return 0;
        else{
            _current = subDir;
            //更新当前路径�?
            if (path[0] == '/')
            {
                current_path = path;
            }
            else
            {
                if (_current == _root)
                {
                    //直接加path;
                    current_path += path;
                }
                else
                {
                    current_path += '/' + path;
                }
            }
        }

        return 1;   //提示弢�发��，进入目录，不用打印信息，显示效果就好；好好显示一�?
    }
    else
    {
        name = "������Ŀ¼�����ڣ�";
        return -1;       //提示弢�发��，目录不存在；
    }
}

//返回上一级目录；
void sys_cdback() {
    current_path = current_path.substr(0, current_path.length() - strlen(_current->_name) );
    string dirName = "";             //用户想要查看的目录的名字；在下面函数获得�?
    const int dir_type = 1;            //丢�定是目录�?
    //霢�要知道用户想查看的目录存不存在；
    FCB*  parentDir = NULL;      //parentDir父目录，目前是不知道的，扢�以设置为NULL，��过下述过程，可以知道parentDir是否存在，以及想查看的目录是否存在；
    int B,F;
    _current = sys_returnFCB(current_path,dirName,dir_type,parentDir,B,F);
}

//剪切�?
int sys_cut(string oldPath, int type, string newPath) {
    //先判读用户给定的路径是否合法；即能否找到这个文件/目录�?
    //type,newPath有开发��询问用户后，获得；

    FCB *parentDir = NULL;      //parentDir父目录，目前是不知道的，扢�以设置为NULL，��过下述过程，可以知道parentDir是否存在，以及想剪切的文件是否存在；
    string cutFCB_name;
    int Bo, Fo;
    FCB *cut_FCB = sys_returnFCB(oldPath, cutFCB_name, type, parentDir, Bo, Fo);
    //如果要剪切的文件FCB不存在，那么返回文件不存�?
    if (cut_FCB != NULL) {
        if (cut_FCB->_access[current_usernum] == '3' || cut_FCB->_access[current_usernum] ==
                                                        '4') {            /************************************************************************/
            //可剪切粘贴；
            //首先判断；粘贴到的目录中，重复利用tmp，判断是否存在同名同类型文件/目录�?
            //parentDir，cutFCB_name会根据path进行修改，可重复利用�?
            //上面的oldPath是包括剪切的文件/目录的；这里的newPath是只霢�要提供要粘贴的目录就行；扢�以tmp此时是个父目录；
            FCB *DestinationDir_parentDir = NULL;
            int Bn, Fn;
            FCB *DestinationDir = sys_returnFCB(newPath, cutFCB_name, 1, DestinationDir_parentDir, Bn,
                                                Fn);      //某个目录中；

            //判断目标目录是否存在�?
            if (DestinationDir == NULL) {
                //目标目录不存在；则无法黏贴；
                return -4;              //提示弢�发��，目标目录不存在；
            } else {
                //目标目录存在，判读其中是否存在同名同类型的对象；
                //查找这个文件|目录的前驱；
                FCB *tmp;
                int Bd, Fd;
                tmp = returnSonFCB(DestinationDir, cut_FCB->_name, cut_FCB->_type, Bd, Fd);
                if (tmp == NULL) {
                    //说明不存在同名同类型的对象；可以黏贴；摘除之前目录树的结�?
                    add_dirOrFile(cut_FCB, DestinationDir);
                    delete_dirOrFile(num2FCBBlock(Bo), Fo);

                    return 1;          //提示弢�发��，剪切粘贴成功�?
                } else {
                    return -1;       //提示弢�发��，该目录下存在同名同类型，不允许粘贴；
                }
            }

        } else return 0;
    }
    else
    {
        if (parentDir == NULL)
        {
            return -2;          //提示弢�发��，想要剪切的文件的父目录不存在�?
        }
        else
        {
            //说明想要剪切的文件不存在�?
            return -3;               //提示弢�发��，由开发��告知用户，剪切的文件不存在�?
        }
    }
}

//复制�?
int sys_copy(string oldPath, int type, string newPath) {
    //先判读用户给定的路径是否合法；即能否找到这个文件/目录�?
    //type,有开发��询问用户后，获得；

    FCB *parentDir = NULL;      //parentDir父目录，目前是不知道的，扢�以设置为NULL，��过下述过程，可以知道parentDir是否存在，以及想剪切的文件是否存在；
    string copyFCB_name;
    int Bo, Fo;
    FCB *copy_FCB = sys_returnFCB(oldPath, copyFCB_name, type, parentDir, Bo, Fo);

    if (copy_FCB != NULL) {
        if (copy_FCB->_access[current_usernum] == '3' || copy_FCB->_access[current_usernum] == '4') {
            //想要复制的存在；
            //复制时，获取要复制首地址即可，但粘贴要注意，拷贝丢�份和原来丢�样的，增加FCB弢�锢�，block弢�锢�；且目录的brother要置空；
            //_copyNode = copy_FCB;


            /**********************************************************/
            //进入复制粘贴�?
            int Bn, Fn;
            //先判断要复制到的那个目录中，是否存在同名同类型的文件�?
            //copyFCB_name是纯粹打酱油的；
            FCB *DestinationDir = sys_returnFCB(newPath, copyFCB_name, 1, parentDir, Bn, Fn);      //某个目录中；
            if (DestinationDir == NULL) {//目标目录不存在；无法进行复制粘贴�?
                return -5;           //提示弢�发��目标目录不存在�?
            } else {
                //目标目录存在�?
                //进入目标目录，判断是否存在同名同类型的对象；
                FCB *tmp;
                int Bd, Fd;
                tmp = returnSonFCB(DestinationDir, copy_FCB->_name, copy_FCB->_type, Bd, Fd);
                if (tmp == NULL) {
                    //用户想复制到的目录中没有同名同类型；
                    //本质就是增加FCB，已经BlLOCK;
                    //由于资源有限，所以需判断丢�个需要增加的FCB和BLOCK够不够；

                    //numFCB = 0; numBLOCK = 0;

                    //调用计算系统资源�?
                    int numBLOCK = num_dirOrFile(copy_FCB);

                    if (numBLOCK <= _emptyBLOCK_Count) {
                        //说明系统资源够用；可以复制；
                        add_dirOrFile(copy_FCB, DestinationDir);
                        return 1;          //提示弢�发��， 复制粘贴成功�?
                    } else {
                        return -1;     //提示弢�发��，系统资源不够，无法进行复制；
                    }

                } else {
                    return -2;       //提示弢�发��，该目录下存在同名同类型，不允许粘贴；
                }
            }
        } else return 0;

    } else {
        if (parentDir == NULL) {
            return -3;          //提示弢�发��，想要复制的文件的父目录不存在�?
        } else {
            //说明想要复制的文件不存在�?
            return -4;               //提示弢�发��，由开发��告知用户，复制的文件不存在�?
        }
    }
}


int sys_share(string oldPath, int type, string newPath) {
    //先判读用户给定的路径是否合法；即能否找到这个文件/目录�?
    //type,有开发��询问用户后，获得；

    FCB *parentDir = NULL;      //parentDir父目录，目前是不知道的，扢�以设置为NULL，��过下述过程，可以知道parentDir是否存在，以及想剪切的文件是否存在；
    string copyFCB_name;
    int Bo, Fo;
    FCB *copy_FCB = sys_returnFCB(oldPath, copyFCB_name, type, parentDir, Bo, Fo);

    if (copy_FCB != NULL) {
        if (copy_FCB->_access[current_usernum] == '3' || copy_FCB->_access[current_usernum] == '4') {
            //想要复制的存在；
            //复制时，获取要复制首地址即可，但粘贴要注意，拷贝丢�份和原来丢�样的，增加FCB弢�锢�，block弢�锢�；且目录的brother要置空；
            //_copyNode = copy_FCB;

            /**********************************************************/
            //进入复制粘贴�?
            int Bn, Fn;
            //先判断要复制到的那个目录中，是否存在同名同类型的文件�?
            //copyFCB_name是纯粹打酱油的；
            FCB *DestinationDir = sys_returnFCB(newPath, copyFCB_name, 1, parentDir, Bn, Fn);      //某个目录中；
            if (DestinationDir == NULL) {//目标目录不存在；无法进行复制粘贴�?
                return -5;           //提示弢�发��目标目录不存在�?
            } else {
                //目标目录存在�?
                //进入目标目录，判断是否存在同名同类型的对象；
                FCB *tmp;
                int Bd, Fd;
                tmp = returnSonFCB(DestinationDir, copy_FCB->_name, copy_FCB->_type, Bd, Fd);
                if (tmp == NULL) {
                    //用户想复制到的目录中没有同名同类型；
                    //本质就是增加FCB，已经BlLOCK;
                    //由于资源有限，所以需判断丢�个需要增加的FCB和BLOCK够不够；

                    //numFCB = 0; numBLOCK = 0;

                    //调用计算系统资源�?
                    int numBLOCK = num_dirOrFile(copy_FCB);

                    if (numBLOCK <= _emptyBLOCK_Count) {
                        //说明系统资源够用；可以复制；
                        int s=share_dirOrFile(copy_FCB, DestinationDir);
                        if(s==1) return 1;          //提示弢�发��， 复制粘贴成功�?
                        else return 0;
                    } else {
                        return -1;     //提示弢�发��，系统资源不够，无法进行复制；
                    }

                } else {
                    return -2;       //提示弢�发��，该目录下存在同名同类型，不允许粘贴；
                }
            }
        } else return 0;

    } else {
        if (parentDir == NULL) {
            return -3;          //提示弢�发��，想要复制的文件的父目录不存在�?
        } else {
            //说明想要复制的文件不存在�?
            return -4;               //提示弢�发��，由开发��告知用户，复制的文件不存在�?
        }
    }
}




int sys_setaccess(string path,char x){
    string name1;
    int type,B,F;
    FCB *p;
    FCB* fcb=sys_returnFCB(path,name1,0,p,B,F);
    if(fcb->_access[current_usernum]=='4') {
        int i=0;
        while(i< _maxUsers){
            if(i!=0||i!=current_usernum)
                fcb->_access[current_usernum]=x;
            i++;
        }
        return 1;
    }else return 0;
}

int main() {
    sys_initDisk();
    string name1;
    int type,B,F;
    FCB *p;
    sys_mkdir("/m") ;
    StringList* ss=sys_dir("");
    cout<<ss->content<<endl;
    cout<<ss->next->content<<endl;


/*
    //if(sys_cd("/m",name)==1) cout<<"cd"<<endl;
    sys_create_file("/m/t");
    FCB* file= sys_returnFCB("/m/t",name1,0,p,B,F);
    if (sys_overwrite_file(file,"hello") == 1) ;
    sys_copy("/m/t",0,"/");


    FCB* file1= sys_returnFCB("/t",name1,0,p,B,F);
    //cout<<sys_read_file(file1);
    //sys_appendwrite_file(file1,"world");
    //cout<<sys_read_file(file1);
    string name;
    //sys_rmdir("/m");
    ///sys_delete_file("/m/t");
//    cout<<sys_read_file(file);


    if(sys_createuser("Jiang")==1)cout<<"user";
    if(sys_su("Jiang")==1) cout<<"su";
    bool flag;
    string s=sys_read_file(file1,flag);
    if(flag)cout<<s;
    else cout<<"fobidden";
*/
}