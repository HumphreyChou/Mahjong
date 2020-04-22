/*
 * @Author: Xia Hanyu
 * @Date:   2020-04-22 16:40:15
 * @Last Modified by:   Xia Hanyu
 * @Last Modified time: 2020-04-22 21:07:46
 */

/**20200422 Xia_Hanyu Update
 ----------------------------------------
 * 修复LastID() bug
 * 对Out() 进行补充，使得能够返回打牌者ID
 * 增加自己鸣牌库pack并提供相关函数
 * 将PrevioInfo()改为Initial()并只进行初始化，其余工作由ProcessKnown()完成
 * ProcessKnown(), 处理之前所有回合明牌并填充自己的鸣牌库
 * 
 ----------------------------------------
 * 下一步的计划：
 * 胡牌/抢杠胡 
 * 我相信今天写的一定有bug 希望能找到
 */

#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
#include <map>
#include <unordered_map>

using namespace std;



/**
 * @brief
 * 全局变量，储存基本牌面信息
 * @param
 * stmp --> string_temp
 * itmp --> int_temp
 * request, response  该回合之前所有信息
 * hand 手牌
 * ostringstream istringstream 用于字符串和其他数据类型之间方便转换
 * myPlayerID 应该在吃牌的时候有用
 * quan 不知道有啥用
 * lastout 储存上一回合打出了什么牌
 * its --> int to string 
 * sti --> string to int 
*/
int turnID;
string stmp;
int itmp;
int idtmp;
vector<string> request, response;
vector<string> hand;
ostringstream sout;
istringstream sin;
string lastout;
int myPlayerID, quan;
string its[34] = { "W1","W2","W3","W4","W5","W6","W7","W8","W9",
                   "B1","B2","B3","B4","B5","B6","B7","B8","B9",
                   "T1","T2","T3","T4","T5","T6","T7","T8","T9",
                   "F1","F2","F3","F4","J1","J2","J3" };

map<string, int> sti;

/**
 * @brief
 * 以下内容用于储存已经出现的牌的出现次数
 * 1. 打出的牌，无论是否被吃、碰，都已不会再变为未知
 * 2. 打出的牌若是被吃、碰、杠，则该顺/刻就永远已知
 * @param
 * known 储存已知牌的哈希表
 * @function
 * AddKnown 将新出现的明牌加入known
*/
unordered_map<string, int> known;
void AddKnown(string card)
{
    auto it = known.find(card);
    if (it == known.end()) {
        known[card] = 1;
    }
    else {
        known[card]++;
    }
}


/**
 * @brief
 * 以下内容存储自己鸣牌（吃、碰、杠得到的牌）
 * @param
 * pack 容器，为了与算番库匹配，定义为 vector<pair<string, pair<string, int> > > 详见算番库介绍
 * @function
 * AddPack 添加鸣牌,并自动加入known
 * CalPos 计算玩家ID相对于自己是上家、对家、下家
*/
vector<pair<string, pair<string, int> > > pack;
void AddPack(string TYPE, string MidCard, int from, bool is_BUGANG = false) // is_BUGANG 用于区分暗杠和补杠
{
    pack.push_back(make_pair(TYPE, make_pair(MidCard, from)));
    // 将自己鸣牌加入known
    switch(TYPE){
        case "PENG": 
            for(int i = 0; i < 3; ++i)
                AddKnown(MidCard);
            break;
        case "CHI":
            AddKnown(MidCard), AddKnown(its[sti[MidCard] - 1]), AddKnown(its[sti[MidCard] + 1]);
            break;
        case "GANG":
            if(is_BUGANG){
                AddKnown(MidCard);
            }else{
                for(int i = 0; i < 4; ++i)
                    AddKnown(MidCard);
            }
    }
}

inline int CalPos(int x)
{
    if(x == myPlayerID) 
        return 0;
    int previous = myPlayerID > 0 ? myPlayerID - 1 : 3;
    int opposite = (myPlayerID + 2) % 4;
    int next = (myPlayerID + 1) % 4;
    switch(x){
        case previous: return 1;
        case opposite: return 2;
        case next: return 3;
    }
}

inline int lastID() 
{
    /**
     * @brief
     * 返回上家ID
    */
    return myPlayerID > 0 ? myPlayerID - 1 : 3;
}

pair<string, int> Out(int k) 
{
    /**
     * @brief
     * 返回k回合打出的牌，如果没有打出，返回NONE
     * 同时也要返回打牌玩家ID, 用于鸣牌时加入
    */
    istringstream ssin;
    int InfoNum;
    int playerID = -1;
    string command;
    string CardName = "NONE";
    
    ssin.str(request[k]);
    ssin >> InfoNum;
    ssin >> playerID;
    if(command >= 3){
        ssin >> command;
        switch(command){
            case "PLAY": 
            case "PENG": ssin >> CardName; break;
            case "CHI": ssin >> CardName >> CardName; break;
            default: break; // BUHUA DRAW GANG BUGANG 没有打出牌
        }
    }
    return make_pair(CardName, playerID);
}

void ProcessKnown()
{
    /**
     * @brief
     * 可以处理除了HU之外的所有信息
     * 1. 将输入进来的所有信息中的明牌加入known哈希表
     * 包括 别人和自己打出的牌、别人的鸣牌、自己的鸣牌，总之已经被固定
     * 2. 将自己的鸣牌放入鸣牌库
    */
    for (int i = 0; i <= turnID; ++i) {
        sin.clear();
        string CardName; 
        // 1. 所有别人出现的明牌
        sin.str(request[i]);
        sin >> itmp;
        if (itmp == 3) {
            sin >> idtmp;
            sin >> stmp;
            if (stmp == "PLAY") {
                sin >> CardName;
                AddKnown(CardName);
            }else if(stmp == "PENG") {
                sin >> CardName;
                AddKnown(CardName);
                AddKnown(Out(i - 1).first);
                AddKnown(Out(i - 1).first);
            }else if(stmp == "CHI") {
                sin >> CardName;
                //为了避免把上回合打出的牌重复加入Known中，暂时先分三种情况处理一下...
                if (Out(i - 1).first == its[sti[CardName] - 1]) { // 1. 吃左牌
                    AddKnown(CardName);
                    AddKnown(its[sti[CardName] + 1]);
                } 
                else if (Out(i - 1).first == its[sti[CardName] + 1]) { // 2. 吃右牌
                    AddKnown(its[sti[CardName] - 1]);
                    AddKnown(CardName);
                }
                else if (Out(i - 1).first == CardName) { // 3. 吃中间
                    AddKnown(its[sti[CardName] - 1]);
                    AddKnown(its[sti[CardName] + 1]);
                }
                sin >> CardName;
                AddKnown(CardName);
            }else if(stmp == "BUGANG") {
                sin >> CardName;
                AddKnown(CardName);
            }else if(stmp == "GANG"){
                // 只处理明杠，因为暗杠也不知道杠的是啥
                if(Out(i - 1).first != "NONE"){
                    for(int j = 0; j < 4; ++j)
                        AddKnown(Out(i - 1).first);
                }
            }
        }
        // 处理自己打出的牌
        if (i == turnID)
            break;
        sin.clear();
        sin.str(response[i]);
        sin >> stmp;
        if(stmp == "PLAY"){
            sin >> CardName;
            AddKnown(CardName);
        }else if(stmp == "PENG"){
            sin >> CardName; // 这是碰后打出的牌
            AddKnown(CardName);
            AddPack("PENG", Out(i).first, CalPos(Out(i).second)); 
        }else if(stmp == "GANG"){
            // 1. 暗杠
            if(Out(i).first == "NONE"){
                sin >> CardName;
                AddPack("GANG", CardName, 0) // 算番库说明没有写暗杠第三个参数应该是什么，先写0吧
            }else{
                // 2. 明杠
                AddPack("GANG", Out(i).first, CalPos(Out(i).second));
            }
        }else if(stmp == "BUGANG"){
            sin >> CardName; // 补杠的牌
            AddPack("GANG", CardName, 0, true);
        }else if(stmp == "CHI"){
            sin >> CardName; // 得到的顺子的中间
            AddPack("CHI", Out(i).first, CalPos(Out(i).second));
            sin >> CardName; // 打出牌
            AddKnown(CardName);
        }
    }
}

void Input()
{
    /**
     * @brief
     * 输入到本回合为止所有信息
    */

    cin >> turnID;
    turnID--;
    getline(cin, stmp);
    // 将之前所有回合输入、输出信息存入
    for (int i = 0; i < turnID; i++) {
        getline(cin, stmp);
        request.push_back(stmp);
        getline(cin, stmp);
        response.push_back(stmp);
    }
    // 本回合输入
    getline(cin, stmp);
    request.push_back(stmp);
}

void Initialize()
{
    /**
     * @brief
     * 初始化工作，如增减手牌等
    */

    // 初始化map sti
    for (int i = 0; i < 34; ++i)
        sti[its[i]] = i;
    // 初始化手牌
    if (turnID < 2) {
        sin.clear();
        return; // 在之后的回合再处理
    } else {
        // 存入初始信息
        sin.str(request[0]); // 第一回合
        sin >> itmp >> myPlayerID >> quan;
        sin.clear();
        sin.str(request[1]); // 第二回合
        for (int j = 0; j < 5; j++) sin >> itmp; // 编号 四个玩家花牌数
        for (int j = 0; j < 13; j++) { // 手牌
            sin >> stmp;
            hand.push_back(stmp);
        }
        for (int i = 2; i < turnID; i++) { // 一二之后每回合的输入输出(不包括本回合)
            sin.clear();
            sin.str(request[i]);
            sin >> itmp; // 输入指令编号
            if (itmp == 2) { // 摸牌
                sin >> stmp;
                hand.push_back(stmp);
                sin.clear();
                sin.str(response[i]);
                sin >> stmp >> stmp;
                hand.erase(find(hand.begin(), hand.end(), stmp));
            }
        }
        sin.clear();
    }
}


void Act()
{
    /**
     * @brief
     * 做出决策
     * 决策取决于本回合输入是什么(编号为2/3)
    */
    sin.clear();
    if (turnID < 2) {
        sout << "PASS";
    }
    else {
        sin.str(request[turnID]); // 本回合输入信息
        sin >> itmp;
        /**
         * 考虑把以下决策内容封装为函数 Draw() Peng() Chi()...
         * 目前功能简单，先这么写吧
        */
        if (itmp == 2) { // 摸牌
            sin >> stmp;
            hand.push_back(stmp);
            /**
             * 此处添加算法
             * 暂时是傻傻的随机出牌
            */
            random_shuffle(hand.begin(), hand.end());
            sout << "PLAY " << *hand.rbegin();
            hand.pop_back();
        } else {
            sin >> idtmp;
            sin >> stmp;
            //下面一行是必定会PASS的情况（补花、摸牌、杠或request来自自己）
            if (idtmp == myPlayerID || stmp == "BUHUA" || stmp == "DRAW" || stmp == "GANG")
                sout << "PASS";
            else {
                if (stmp == "BUGANG") {
                    //判断是否能够抢杠胡、是否抢杠胡
                }
                string ThisOut = Out(turnID);
                if (idtmp == lastID()) {
                    //判断是否能吃和是否要吃
                }
                //判断是否能碰、杠、胡和是否要胡
            }
        }
    }
    response.push_back(sout.str());
    cout << response[turnID] << endl;
}

int main()
{
    Input();
    Initialize();
    ProcessKnown();
    Act();
    system("pause");
    return 0;
}