/*
 * @Author: Xia Hanyu
 * @Date:   2020-05-05 10:55:15
 * @Last Modified by:   Xia Hanyu
 * @Last Modified time: 2020-05-05 12:13:48
 */
 /**20200505 Zhou Yuhan Update
  ----------------------------------------
  * 修复了AddPack() bug,将pack从手牌里移除
  * 添加了CHI() PENG() BUGANG() GANG()，可以处理鸣牌
  * 将权值转为struct params 便于以后调参
  ----------------------------------------
  * 下一步的计划：
  * 把算法函数加入Act()
  * 优化marking()的参数
  */

#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
#include <map>
#include <set>
#include <unordered_map>
#include "MahjongGB/MahjongGB.h"

using namespace std;


/**
 * @brief
 * 全局变量，储存基本牌面信息
 * @param
 * stmp --> string_temp
 * itmp --> int_temp
 * request, response  该回合之前所有信息
 * hand 手牌
 * handset 储存纯数字化的手牌
 * ostringstream istringstream 用于字符串和其他数据类型之间方便转换
 * myPlayerID 应该在吃牌的时候有用
 * quan 不知道有啥用
 * flowerCount 自己补花数
 * TileLeft 牌墙剩余牌数
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
multiset<int> handset;
ostringstream sout;
istringstream sin;
string lastout;
int myPlayerID, quan;
int flowerCount;
int TileLeft = 92; // 144 - 13 * 4
string its[34] = { "W1","W2","W3","W4","W5","W6","W7","W8","W9",
                   "B1","B2","B3","B4","B5","B6","B7","B8","B9",
                   "T1","T2","T3","T4","T5","T6","T7","T8","T9",
                   "F1","F2","F3","F4","J1","J2","J3" };

map<string, int> sti;

int NewNum(string name) {
    /**
     * @brief
     * 将牌名转换为数字
     * 用11-19表示W1-W9 31-39表示B1-B9 51-59表示T1-T9 F1=65 F2=70 F3=75 F4=80 J1=85 J2=90 J3=95
    */
    char a1 = name[0];
    char a2 = name[1];
    if (a1 == 'W') 
        return a2 - '0' + 10;
    if (a1 == 'B')
        return a2 - '0' + 30;
    if (a1 == 'T')
        return a2 - '0' + 50;
    if (a1 == 'F')
        return (a2 - '0') * 5 + 60;
    return (a2 - '0') * 5 + 80;
}

//用来把hand里的string转化成handset里的int储存起来方便评估每张牌的权重
void HandsetInit() {
    int len = hand.size();
    for (int i = 0; i < len; ++i) {
        string card = hand[i];
        handset.insert(NewNum(card));
    }
}

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
 * 用于计算牌面权值的参数
 * @param
 * params 储存参数
*/
struct node
{
    int same;
    int adjacent;
    int interval;
    int unknown;
};

node params;

//给hand里的每张牌评分
int Marking(string name) {
    int x = NewNum(name);
    int mark = 0;
    int n20 = handset.count(x - 2);
    int n10 = handset.count(x - 1);
    int n00 = handset.count(x);
    int n01 = handset.count(x + 1);
    int n02 = handset.count(x + 2);
    mark += n00 * params.same;//相同牌的权重
    mark += (n10 > 0) * params.adjacent;
    mark += (n01 > 0) * params.adjacent;//相邻牌的权重
    mark += (n10 > 0) * params.interval;
    mark += (n01 > 0) * params.interval;//隔牌的权重
    
    // 处理还没有的牌的权重
    int k20 = 0;
    if (sti[name] - 2 >= 0) {
        auto it = known.find(its[sti[name] - 2]);
        if (it != known.end())
            k20 = 4 - known[its[sti[name] - 2]];
        else
            k20 = 4;
    } 
    int k10 = 0;
    if (sti[name] - 1 >= 0) {
        auto it = known.find(its[sti[name] - 1]);
        if (it != known.end())
            k10 = 4 - known[its[sti[name] - 1]];
        else
            k10 = 4;
    }
    int k00 = 4 - known[name];
    int k02 = 0;
    if (sti[name] + 2 < 27) {
        auto it = known.find(its[sti[name] + 2]);
        if (it != known.end())
            k02 = 4 - known[its[sti[name] + 2]];
        else
            k02 = 4;
    }
    int k01 = 0;
    if (sti[name] + 1 < 27) {
        auto it = known.find(its[sti[name] + 1]);
        if (it != known.end())
            k01 = 4 - known[its[sti[name] + 1]];
        else
            k01 = 4;
    }
    if (n20 > 0 && n10 == 0)//(x-2)(x)的情况
        mark += k10 * params.unknown;
    if (n10 > 0 && n20 == 0)//(x-1)(x)的情况
        mark += (k20 * params.unknown + k01 * params.unknown);
    if (n02 > 0 && n01 == 0)//(x)(x+2)的情况
        mark += k01 * params.unknown;
    if (n01 > 0 && n02 == 0)//(x)(x+1)的情况
        mark += (k02 * params.unknown + k10 * params.unknown);
    if (n00 == 2)//(x)(x)的情况
        mark += k00 * params.unknown;
    return mark;
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
void AddPack(string TYPE, string MidCard, int from = 0, bool is_BUGANG = false) // is_BUGANG 用于区分暗杠和补杠
{
    pack.push_back(make_pair(TYPE, make_pair(MidCard, from)));
    // 将自己鸣牌加入known, 并把pack从手牌里拿掉
    if (TYPE == "PENG") {
        for (int i = 0; i < 2; ++i){ // 只需要两次，因为该回合request打出的牌已经添加过一次
            AddKnown(MidCard);
            // vector没有find()，很伤心
            for(auto j = hand.begin(); j != hand.end(); ++j){
                if(*j == MidCard){
                    hand.erase(j); break;
                }
            }
        }
    }
    else if (TYPE == "CHI") {
        int CHIcard_code = from - 2; // 1 2 3 --> -1 0 1
        for (int i = -1; i <= 1; ++i) {
            if (i != CHIcard_code){
                AddKnown(its[sti[MidCard] + i]);
                for(auto j = hand.begin(); j != hand.end(); ++j){
                    if(*j == its[sti[MidCard] + i]){
                        hand.erase(j); break;
                    }
                }
            }
        }
    }
    else if (TYPE == "GANG") {
        if (is_BUGANG) {
            AddKnown(MidCard);
            for(auto j = hand.begin(); j != hand.end(); ++j){
                if(*j == MidCard){
                    hand.erase(j); break;
                }
            }
        }
        else {
            if (from == 0) { // 暗杠
                for (int i = 0; i < 4; ++i){
                    AddKnown(MidCard);
                    for(auto j = hand.begin(); j != hand.end(); ++j){
                        if(*j == MidCard){
                            hand.erase(j); break;
                        }
                    }
                }
            }
            else { // 明杠
                for (int i = 0; i < 3; ++i){
                    AddKnown(MidCard);
                    for(auto j = hand.begin(); j != hand.end(); ++j){
                        if(*j == MidCard){
                            hand.erase(j); break;
                        }
                    }
                }
            }
        }
    }
}

inline int CalPos(int x)
{
    if (x == myPlayerID)
        return 0;
    int previous = myPlayerID > 0 ? myPlayerID - 1 : 3;
    int opposite = (myPlayerID + 2) % 4;
    int next = (myPlayerID + 1) % 4;
    if (x == previous) {
        return 1;
    }
    else if (x == opposite) {
        return 2;
    }
    else if (x == next) {
        return 3;
    }
}

inline int CalSupply(string MidCard, string SupplyCard)
{
    return 2 - (sti[MidCard] - sti[SupplyCard]);
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
    if (InfoNum >= 3) {
        ssin >> command;
        if (command == "PLAY" || command == "PENG") {
            ssin >> CardName;
        }
        else if (command == "CHI") {
            ssin >> CardName >> CardName;
        }
    }
    return make_pair(CardName, playerID);
}

/**
 * @brief
 * 以下内容是算法AI部分，可能需要大幅修改
 * @function
 * 1. HU()判断是否能胡
 * 2. CHI()判断是否能吃
 * 3. PENG()判断是否能碰
 * 4. BUGANG()判断是否能补杠，因为补杠输出信息不同于明暗杠
 * 5. Gang()判断能否明/暗杠
 * @notice
 * 存在一些问题，还没想好怎么处理
 * 1. 仅仅通过判断是否有连牌去判断能否吃/碰/杠，可能导致重复操作，比如已经有 T1 T2 T3,会再吃一张T1，打出T1(也可能打出别的牌)
 * 2. 如果判断能吃/碰/杠就操作了，好像也不太合理
*/
bool HU(string winTile, bool isZIMO, bool isGANG)
{
    MahjongInit(); // 初始化算番库
    bool isJUEZHANG = (known[winTile] == 3);
    bool isLast = (TileLeft == 0);
    vector<pair<int, string> > result = MahjongFanCalculator(
        pack, hand, winTile, flowerCount, isZIMO, isJUEZHANG, isGANG, isLast, myPlayerID, quan
    );
    int FanSum = 0;
    for (auto i = result.begin(); i != result.end(); ++i) {
        FanSum += i->first;
    }
    return FanSum >= 8;
}

bool CHI(string card)
{
    int card_num = NewNum(card);
    bool flag = false;
    string mid_card;
    int from;
    // 1. 吃左牌
    if(handset.find(card_num + 1) != handset.end() && handset.find(card_num + 2) != handset.end()){
        flag = true;
        mid_card = its[sti[card] + 1];
        from = 1;
    }
    // 2. 吃中间
    if(handset.find(card_num - 1) != handset.end() && handset.find(card_num + 1) != handset.end()){
        flag = true;
        mid_card = card;
        from = 2;
    }
    // 3. 吃右牌
    if(handset.find(card_num - 1) != handset.end() && handset.find(card_num - 2) != handset.end()){
        flag = true;
        mid_card = its[sti[card] - 1];
        from = 3;
    }
    if(flag){
        AddPack("CHI", mid_card, from);
    }
    return flag;
}

bool PENG(string card)
{
    int card_num = NewNum(card);
    if(handset.count(card_num) == 2){
        AddPack("PENG", card);
        return true;
    }
    return false;
}

bool BUGANG(string card)
{   
    for(auto i = pack.begin(); i != pack.end(); ++i){
        if(i->first == "PENG" && (*i).second.first == card){
            AddPack("GANG", card, 0, true);
            return true;
        }
    }
    return false;
}

bool GANG(string card, int from)
{
    int card_num = NewNum(card);
    if(handset.count(card_num) == 3){
        AddPack("GANG", card, from);
        return true;
    } 
    return false;
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
        // 1. 所有request出现的明牌
        sin.str(request[i]);
        sin >> itmp;
        if (itmp == 3) {
            sin >> idtmp;
            if (idtmp == myPlayerID)
                continue;
            sin >> stmp;
            if (stmp == "DRAW") {
                TileLeft--;
            }
            else if (stmp == "PLAY") {
                sin >> CardName;
                AddKnown(CardName);
            }
            else if (stmp == "PENG") {
                sin >> CardName;
                AddKnown(CardName);
                AddKnown(Out(i - 1).first);
                AddKnown(Out(i - 1).first);
            }
            else if (stmp == "CHI") {
                sin >> CardName;
                // 避免把上回合打出的牌重复加入Known
                string CHIcard = Out(i - 1).first;
                for (int i = -1; i <= 1; ++i) {
                    if (sti[CardName] + i != sti[CHIcard])
                        AddKnown(its[sti[CardName] + i]);
                }
                sin >> CardName;
                AddKnown(CardName);
            }
            else if (stmp == "BUGANG") {
                sin >> CardName;
                AddKnown(CardName);
            }
            else if (stmp == "GANG") {
                // 只处理明杠，因为暗杠也不知道杠的是啥
                if (Out(i - 1).first != "NONE") {
                    for (int j = 0; j < 4; ++j)
                        AddKnown(Out(i - 1).first);
                }
            }
        }
        // 处理response
        if (i == turnID)
            break;
        sin.clear();
        sin.str(response[i]);
        sin >> stmp;
        if (stmp == "PLAY") {
            sin >> CardName;
            AddKnown(CardName);
        }
        else if (stmp == "PENG") {
            sin >> CardName; // 这是碰后打出的牌
            AddKnown(CardName);
            AddPack("PENG", Out(i).first, CalPos(Out(i).second));
        }
        else if (stmp == "GANG") {
            // 1. 暗杠
            if (Out(i).first == "NONE") {
                sin >> CardName;
                AddPack("GANG", CardName, 0); // 算番库说明没有写暗杠第三个参数应该是什么，先写0吧
            }
            else {
                // 2. 明杠
                AddPack("GANG", Out(i).first, CalPos(Out(i).second));
            }
        }
        else if (stmp == "BUGANG") {
            sin >> CardName; // 补杠的牌
            AddPack("GANG", CardName, 0, true);
        }
        else if (stmp == "CHI") {
            sin >> CardName; // 得到的顺子的中间
            AddPack("CHI", CardName, CalSupply(CardName, Out(i).first));
            sin >> CardName; // 打出牌
            AddKnown(CardName);
        }
    }
    // 此时将手牌变为纯数字形式
    HandsetInit();
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
     * 初始化工作
    */

    // 初始化map sti
    for (int i = 0; i < 34; ++i)
        sti[its[i]] = i;
    // 初始化手牌
    if (turnID < 2) {
        sin.clear();
        return; // 在之后的回合再处理
    }
    else {
        // 存入初始信息
        sin.str(request[0]); // 第一回合
        sin >> itmp >> myPlayerID >> quan;
        sin.clear();
        sin.str(request[1]); // 第二回合
        sin >> itmp; // 编号
        for (int j = 0; j < 4; j++) {
            sin >> itmp; // 四个玩家花牌数
            TileLeft -= itmp; // 补花减少牌墙牌的数量
            if (j == myPlayerID)
                flowerCount = itmp;
        }
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
        }
        else {
            sin >> idtmp;
            sin >> stmp;
            //下面一行是必定会PASS的情况（补花、摸牌、杠或request来自自己）
            if (idtmp == myPlayerID || stmp == "BUHUA" || stmp == "DRAW" || stmp == "GANG") {
                sout << "PASS";
            }
            else {
                if (stmp == "BUGANG") {
                    //判断是否能够抢杠胡、是否抢杠胡
                }
                pair<string, int> ThisOut = Out(turnID);
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