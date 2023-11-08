// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Copyright (c) 2014-2015 The Dash developers
// Copyright (c) 2015-2020 The RPDCHAIN developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "rpc/client.h"

#include "rpc/protocol.h"
#include "guiinterface.h"
#include "util.h"

#include <set>
#include <stdint.h>

#include <boost/algorithm/string/case_conv.hpp> // for to_lower()
#include <univalue.h>


class CRPCConvertParam
{
public:
    std::string methodName; //! method whose params want conversion
    int paramIdx;           //! 0-based idx of param to convert
};
// ***TODO***
static const CRPCConvertParam vRPCConvertParams[] =
    {
        {"stop", 0},
        {"setmocktime", 0},
        {"getaddednodeinfo", 0},
        {"setgenerate", 0},
        {"setgenerate", 1},
        {"generate", 0},
        {"getnetworkhashps", 0},
        {"getnetworkhashps", 1},
        {"delegatestake", 1},
        {"delegatestake", 3},
        {"delegatestake", 4},
        {"delegatestake", 5},
        {"rawdelegatestake", 1},
        {"rawdelegatestake", 3},
        {"rawdelegatestake", 4},
        {"sendtoaddress", 1},
        {"sendtoaddressix", 1},
        {"settxfee", 0},
        {"getreceivedbyaddress", 1},
        {"getreceivedbyaccount", 1},
        {"getreceivedbylabel", 1},
        {"listcoldutxos", 0},
        {"listdelegators", 0},
        {"listreceivedbyaddress", 0},
        {"listreceivedbyaddress", 1},
        {"listreceivedbyaddress", 2},
        {"listreceivedbyaccount", 0},
        {"listreceivedbyaccount", 1},
        {"listreceivedbyaccount", 2},
        {"listreceivedbylabel", 0},
        {"listreceivedbylabel", 1},
        {"listreceivedbylabel", 2},
        {"getbalance", 1},
        {"getbalance", 2},
        {"getbalance", 3},
        {"getblockhash", 0},
        { "waitforblockheight", 0 },
        { "waitforblockheight", 1 },
        { "waitforblock", 1 },
        { "waitforblock", 2 },
        { "waitfornewblock", 0 },
        { "waitfornewblock", 1 },
        {"move", 2},
        {"move", 3},
        {"sendfrom", 2},
        {"sendfrom", 3},
        {"listtransactions", 1},
        {"listtransactions", 2},
        {"listtransactions", 3},
        {"listtransactions", 4},
        {"listtransactions", 5},
        {"listaccounts", 0},
        {"listaccounts", 1},
        {"walletpassphrase", 1},
        {"walletpassphrase", 2},
        {"getblocktemplate", 0},
        {"listsinceblock", 1},
        {"listsinceblock", 2},
        {"sendmany", 1},
        {"sendmany", 2},
        {"addmultisigaddress", 0},
        {"addmultisigaddress", 1},
        {"createmultisig", 0},
        {"createmultisig", 1},
        {"listunspent", 0},
        {"listunspent", 1},
        {"listunspent", 2},
        {"listunspent", 3},
        {"logging", 0},
        {"logging", 1},
        {"getblock", 1},
        {"getblockheader", 1},
        {"gettransaction", 1},
        {"getrawtransaction", 1},
        {"createrawtransaction", 0},
        {"createrawtransaction", 1},
        {"createrawtransaction", 2},
        {"fundrawtransaction", 1},
        {"signrawtransaction", 1},
        {"signrawtransaction", 2},
        {"sendrawtransaction", 1},
        {"sendrawtransaction", 2},
        {"sethdseed", 0},
        {"gettxout", 1},
        {"gettxout", 2},
        {"lockunspent", 0},
        {"lockunspent", 1},
        {"importprivkey", 2},
        {"importprivkey", 3},
        {"importaddress", 2},
        {"importaddress", 3},
        {"importpubkey", 2},
        {"verifychain", 0},
        {"verifychain", 1},
        {"keypoolrefill", 0},
        {"getrawmempool", 0},
        {"estimatefee", 0},
        {"estimatesmartfee", 0},
        {"prioritisetransaction", 1},
        {"prioritisetransaction", 2},
        {"setban", 2},
        {"setban", 3},
        {"spork", 1},
        {"preparebudget", 2},
        {"preparebudget", 3},
        {"preparebudget", 5},
        {"submitbudget", 2},
        {"submitbudget", 3},
        {"submitbudget", 5},
        {"submitbudget", 7},
        // disabled until removal of the legacy 'masternode' command
        //{"startmasternode", 1},
        {"mnvoteraw", 1},
        {"mnvoteraw", 4},
        {"setstakesplitthreshold", 0},
        {"autocombinerewards", 0},
        {"autocombinerewards", 1},
        {"getzerocoinbalance", 0},
        {"listmintedzerocoins", 0},
        {"listmintedzerocoins", 1},
        {"listspentzerocoins", 0},
        {"listzerocoinamounts", 0},
        {"mintzerocoin", 0},
        {"mintzerocoin", 1},
        {"spendzerocoin", 0},
        {"spendrawzerocoin", 2},
        {"spendzerocoinmints", 0},
        {"importzerocoins", 0},
        {"exportzerocoins", 0},
        {"exportzerocoins", 1},
        {"resetmintzerocoin", 0},
        {"getspentzerocoinamount", 1},
        {"generatemintlist", 0},
        {"generatemintlist", 1},
        {"searchdzrpd", 0},
        {"searchdzrpd", 1},
        {"searchdzrpd", 2},
        {"getmintsvalues", 2},
        {"enableautomintaddress", 0},
        {"getblockindexstats", 0},
        {"getblockindexstats", 1},
        {"getblockindexstats", 2},
        {"getserials", 0},
        {"getserials", 1},
        {"getserials", 2},
        {"getfeeinfo", 0},
        {"getaddressutxos", 1},
        {"getaddressutxos", 2},

        /* Token Core - data retrieval calls */
        { "token_gettradehistoryforaddress", 1 },
        { "token_gettradehistoryforaddress", 2 },
        { "token_gettradehistoryforpair", 0 },
        { "token_gettradehistoryforpair", 1 },
        { "token_gettradehistoryforpair", 2 },
        { "token_setautocommit", 0 },
        { "gettokencrowdsale", 0 },
        { "gettokencrowdsale", 1 },
        { "token_getgrants", 0 },
        { "gettokenbalance", 1 },
        { "gettoken", 0 },
        { "listtokentransactions", 1 },
        { "listtokentransactions", 2 },
        { "listtokentransactions", 3 },
        { "listtokentransactions", 4 },
        { "getalltokenbalances", 0 },
        { "listblocktokentransactions", 0 },
        { "listblockstokentransactions", 0 },
        { "listblockstokentransactions", 1 },
        { "token_getorderbook", 0 },
        { "token_getorderbook", 1 },
        { "gettokenseedblocks", 0 },
        { "gettokenseedblocks", 1 },
        { "token_getmetadexhash", 0 },
        { "token_getfeecache", 0 },
        { "token_getfeeshare", 1 },
        { "token_getfeetrigger", 0 },
        { "token_getfeedistribution", 0 },
        { "token_getfeedistributions", 0 },
        { "gettokenbalanceshash", 0 },
        { "getwallettokenbalances", 0 },
        { "getwalletaddresstokenbalances", 0 },

        /* Token Core - transaction calls */
        { "token_sendsto", 1 },
        { "token_sendsto", 4 },
        { "sendalltokens", 2 },
        { "token_sendtrade", 1 },
        { "token_sendtrade", 3 },
        { "token_sendcanceltradesbyprice", 1 },
        { "token_sendcanceltradesbyprice", 3 },
        { "token_sendcanceltradesbypair", 1 },
        { "token_sendcanceltradesbypair", 2 },
        { "token_sendcancelalltrades", 1 },
        { "sendtokenissuancefixed", 1 },
        { "sendtokenissuancefixed", 2 },
        { "sendtokenissuancefixed", 3 },
        { "sendtokenissuancemanaged", 1 },
        { "sendtokenissuancemanaged", 2 },
        { "sendtokenissuancemanaged", 3 },
        { "sendtokenissuancecrowdsale", 1 },
        { "sendtokenissuancecrowdsale", 2 },
        { "sendtokenissuancecrowdsale", 3 },
        { "sendtokenissuancecrowdsale", 11 },
        { "sendtokenissuancecrowdsale", 12 },
        { "sendtokenissuancecrowdsale", 13 },
        { "token_senddexsell", 1 },
        { "token_senddexsell", 4 },
        { "token_senddexsell", 6 },
        { "token_senddexaccept", 2 },
        { "token_senddexaccept", 4 },
        { "token_sendclosecrowdsale", 1 },
        { "token_sendgrant", 2 },
        { "token_sendrevoke", 1 },
        { "token_sendchangeissuer", 2 },
        { "token_sendenablefreezing", 1 },
        { "token_senddisablefreezing", 1 },
        { "sendtokenfreeze", 2 },
        { "sendtokenunfreeze", 2 },
        { "token_senddeactivation", 1 },
        { "token_sendactivation", 1 },
        { "token_sendactivation", 2 },
        { "token_sendactivation", 3 },
        { "token_sendalert", 1 },
        { "token_sendalert", 2 },
        { "token_funded_send", 2 },
        { "token_funded_sendall", 2 },

        /* Token Core - raw transaction calls */
        { "decodetokentransaction", 1 },
        { "decodetokentransaction", 2 },
        { "createrawtokentxreference", 2 },
        { "createrawtokentxinput", 2 },
        { "createrawtokentxchange", 1 },
        { "createrawtokentxchange", 3 },
        { "createrawtokentxchange", 4 },

        /* Token Core - payload creation */
        { "token_createpayload_simplesend", 0 },
        { "token_createpayload_sendall", 0 },
        { "token_createpayload_dexsell", 0 },
        { "token_createpayload_dexsell", 3 },
        { "token_createpayload_dexsell", 5 },
        { "token_createpayload_dexaccept", 0 },
        { "token_createpayload_sto", 0 },
        { "token_createpayload_sto", 2 },
        { "token_createpayload_issuancefixed", 0 },
        { "token_createpayload_issuancefixed", 1 },
        { "token_createpayload_issuancefixed", 2 },
        { "token_createpayload_issuancemanaged", 0 },
        { "token_createpayload_issuancemanaged", 1 },
        { "token_createpayload_issuancemanaged", 2 },
        { "token_createpayload_issuancecrowdsale", 0 },
        { "token_createpayload_issuancecrowdsale", 1 },
        { "token_createpayload_issuancecrowdsale", 2 },
        { "token_createpayload_issuancecrowdsale", 8 },
        { "token_createpayload_issuancecrowdsale", 10 },
        { "token_createpayload_issuancecrowdsale", 11 },
        { "token_createpayload_issuancecrowdsale", 12 },
        { "token_createpayload_closecrowdsale", 0 },
        { "token_createpayload_grant", 0 },
        { "token_createpayload_revoke", 0 },
        { "token_createpayload_changeissuer", 0 },
        { "token_createpayload_trade", 0 },
        { "token_createpayload_trade", 2 },
        { "token_createpayload_canceltradesbyprice", 0 },
        { "token_createpayload_canceltradesbyprice", 2 },
        { "token_createpayload_canceltradesbypair", 0 },
        { "token_createpayload_canceltradesbypair", 1 },
        { "token_createpayload_cancelalltrades", 0 },
        { "token_createpayload_enablefreezing", 0 },
        { "token_createpayload_disablefreezing", 0 },
        { "token_createpayload_freeze", 1 },
        { "token_createpayload_unfreeze", 1 },

        /* Token Core - backwards compatibility */
        { "getcrowdsale_MP", 0 },
        { "getcrowdsale_MP", 1 },
        { "getgrants_MP", 0 },
        { "send_MP", 2 },
        { "getbalance_MP", 1 },
        { "sendtoowners_MP", 1 },
        { "getproperty_MP", 0 },
        { "listtransactions_MP", 1 },
        { "listtransactions_MP", 2 },
        { "listtransactions_MP", 3 },
        { "listtransactions_MP", 4 },
        { "getallbalancesforid_MP", 0 },
        { "listblocktransactions_MP", 0 },
        { "getorderbook_MP", 0 },
        { "getorderbook_MP", 1 },
        { "trade_MP", 1 }, // depreciated
        { "trade_MP", 3 }, // depreciated
        { "trade_MP", 5 }, // depreciated
    };

class CRPCConvertTable
{
private:
    std::set<std::pair<std::string, int> > members;

public:
    CRPCConvertTable();

    bool convert(const std::string& method, int idx)
    {
        return (members.count(std::make_pair(method, idx)) > 0);
    }
};

CRPCConvertTable::CRPCConvertTable()
{
    const unsigned int n_elem =
        (sizeof(vRPCConvertParams) / sizeof(vRPCConvertParams[0]));

    for (unsigned int i = 0; i < n_elem; i++) {
        members.insert(std::make_pair(vRPCConvertParams[i].methodName,
            vRPCConvertParams[i].paramIdx));
    }
}

static CRPCConvertTable rpcCvtTable;

/** Non-RFC4627 JSON parser, accepts internal values (such as numbers, true, false, null)
 * as well as objects and arrays.
 */
UniValue ParseNonRFCJSONValue(const std::string& strVal)
{
    UniValue jVal;
    if (!jVal.read(std::string("[")+strVal+std::string("]")) ||
        !jVal.isArray() || jVal.size()!=1)
        throw std::runtime_error(std::string("Error parsing JSON:")+strVal);
    return jVal[0];
}

/** Convert strings to command-specific RPC representation */
UniValue RPCConvertValues(const std::string &strMethod, const std::vector<std::string> &strParams)
{
    UniValue params(UniValue::VARR);

    for (unsigned int idx = 0; idx < strParams.size(); idx++) {
        const std::string& strVal = strParams[idx];

        if (!rpcCvtTable.convert(strMethod, idx)) {
            // insert string value directly
            params.push_back(strVal);
        } else {
            // parse string as JSON, insert bool/number/object/etc. value
            params.push_back(ParseNonRFCJSONValue(strVal));
        }
    }

    return params;
}
