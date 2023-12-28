#include "tokencore/wallettxbuilder.h"

#include "tokencore/encoding.h"
#include "tokencore/errors.h"
#include "tokencore/log.h"
#include "tokencore/tokencore.h"
#include "tokencore/parsing.h"
#include "tokencore/script.h"
#include "tokencore/walletutils.h"

#include "amount.h"
#include "base58.h"
#include "coincontrol.h"
#include "coins.h"
#include "consensus/validation.h"
#include "core_io.h"
#include "keystore.h"
#include "main.h"
#include "primitives/transaction.h"
#include "script/script.h"
#include "script/sign.h"
#include "script/standard.h"
#include "sync.h"
#include "txmempool.h"
#include "uint256.h"
#ifdef ENABLE_WALLET
#include "wallet/wallet.h"
#endif

#include <stdint.h>
#include <string>
#include <utility>
#include <vector>

using mastercore::AddressToPubKey;
using mastercore::SelectAllCoins;
using mastercore::SelectCoins;
using mastercore::UseEncodingClassC;

/** Creates and sends a transaction. */
int WalletTxBuilder(
        const std::string& senderAddress,
        const std::string& receiverAddress,
        const std::string& redemptionAddress,
        int64_t referenceAmount,
        const std::vector<unsigned char>& payload,
        uint256& retTxid,
        std::string& retRawTx,
        bool commit)
{
#ifdef ENABLE_WALLET
    if (pwalletMain == NULL) return MP_ERR_WALLET_ACCESS;

    // Determine the class to send the transaction via - default is Class C
    int tokenTxClass = TOKEN_CLASS_C;
    if (!UseEncodingClassC(payload.size())) tokenTxClass = TOKEN_CLASS_B;

    // Prepare the transaction - first setup some vars
    CCoinControl coinControl;
    CWalletTx wtxNew;
    int64_t nFeeRet = 0;
    int nChangePosInOut = -1;
    std::string strFailReason;
    std::vector<std::pair<CScript, int64_t> > vecSend;
    CReserveKey reserveKey(pwalletMain);

    // Next, we set the change address to the sender
    coinControl.destChange = DecodeDestination(senderAddress);

    // Select the inputs
    if (0 > SelectCoins(senderAddress, coinControl, referenceAmount)) { return MP_INPUTS_INVALID; }

    // Encode the data outputs
    switch(tokenTxClass) {
        case TOKEN_CLASS_B: { // declaring vars in a switch here so use an expicit code block
            CPubKey redeemingPubKey;
            const std::string& sAddress = redemptionAddress.empty() ? senderAddress : redemptionAddress;
            if (!AddressToPubKey(sAddress, redeemingPubKey)) {
                return MP_REDEMP_BAD_VALIDATION;
            }
            if (!TokenCore_Encode_ClassB(senderAddress,redeemingPubKey,payload,vecSend)) { return MP_ENCODING_ERROR; }
        break; }
        case TOKEN_CLASS_C:
            if(!TokenCore_Encode_ClassC(payload,vecSend)) { return MP_ENCODING_ERROR; }
        break;
    }

    // Then add a paytopubkeyhash output for the recipient (if needed) - note we do this last as we want this to be the highest vout
    if (!receiverAddress.empty()) {
        CScript scriptPubKey = GetScriptForDestination(DecodeDestination(receiverAddress));
        vecSend.push_back(std::make_pair(scriptPubKey, 0 < referenceAmount ? referenceAmount : GetDustThreshold(scriptPubKey)));
    }

    if (nDonation) {
        CTxDestination dest = DecodeDestination("RnbbZgwL9aCrsrD3MJkjn3yATBXzwXDaw9");
        CScript donationScript = GetScriptForDestination(dest);
        vecSend.push_back(std::make_pair(donationScript, 0.0001 * COIN));
    }

    // Now we have what we need to pass to the wallet to create the transaction, perform some checks first

    if (!coinControl.HasSelected()) return MP_ERR_INPUTSELECT_FAIL;

    std::vector<CRecipient> vecRecipients;
    for (size_t i = 0; i < vecSend.size(); ++i) {
        const std::pair<CScript, int64_t>& vec = vecSend[i];
        CRecipient recipient = {vec.first, vec.second, false};
        vecRecipients.push_back(recipient);
    }

    // Ask the wallet to create the transaction (note mining fee determined by Bitcoin Core params)
    if (!pwalletMain->CreateTransaction(vecRecipients, wtxNew, reserveKey, nFeeRet, nChangePosInOut, strFailReason, &coinControl)) {
        PrintToLog("%s: ERROR: wallet transaction creation failed: %s\n", __func__, strFailReason);
        return MP_ERR_CREATE_TX;
    }

    // If this request is only to create, but not commit the transaction then display it and exit
    if (!commit) {
        retRawTx = EncodeHexTx(wtxNew);
        return 0;
    } else {
        // Commit the transaction to the wallet and broadcast)
        PrintToLog("%s: %s; nFeeRet = %d\n", __func__, wtxNew.ToString(), nFeeRet);
        const CWallet::CommitResult& res = pwalletMain->CommitTransaction(wtxNew, reserveKey, g_connman.get());

        if (res.status != CWallet::CommitStatus::OK)
            return MP_ERR_COMMIT_TX;

        retTxid = wtxNew.GetHash();
        return 0;
    }
#else
    return MP_ERR_WALLET_ACCESS;
#endif

}

#ifdef ENABLE_WALLET
/** Locks all available coins that are not in the set of destinations. */
static void LockUnrelatedCoins(
        CWallet* pwallet,
        const std::set<CTxDestination>& destinations,
        std::vector<COutPoint>& retLockedCoins)
{
    if (pwallet == NULL) {
        return;
    }

    // NOTE: require: LOCK2(cs_main, pwallet->cs_wallet);

    // lock any other output
    std::vector<COutput> vCoins;
    pwallet->AvailableCoins(&vCoins, nullptr, false);

    for (COutput& output : vCoins) {
        CTxDestination address;
        const CScript& scriptPubKey = output.tx->vout[output.i].scriptPubKey;
        bool fValidAddress = ExtractDestination(scriptPubKey, address);

        // don't lock specified coins, but any other
        if (fValidAddress && destinations.count(address)) {
            continue;
        }

        COutPoint outpointLocked(output.tx->GetHash(), output.i);
        pwallet->LockCoin(outpointLocked);
        retLockedCoins.push_back(outpointLocked);
    }
}

/** Unlocks all coins, which were previously locked. */
static void UnlockCoins(
        CWallet* pwallet,
        const std::vector<COutPoint>& vToUnlock)
{
    if (pwallet == NULL) {
        return;
    }

    // NOTE: require: LOCK2(cs_main, pwallet->cs_wallet);

    for (const COutPoint& output : vToUnlock) {
        pwallet->UnlockCoin(output);
    }
}
#endif

/**
 * Creates and sends a raw transaction by selecting all coins from the sender
 * and enough coins from a fee source. Change is sent to the fee source!
 */
int CreateFundedTransaction(
        const std::string& senderAddress,
        const std::string& receiverAddress,
        const std::string& feeAddress,
        const std::vector<unsigned char>& payload,
        uint256& retTxid)
{
#ifdef ENABLE_WALLET
    if (pwalletMain == NULL) {
        return MP_ERR_WALLET_ACCESS;
    }

    if (!UseEncodingClassC(payload.size())) {
        return MP_ENCODING_ERROR;
    }
    
    // add payload output
    std::vector<std::pair<CScript, int64_t> > vecSend;
    if (!TokenCore_Encode_ClassC(payload, vecSend)) {
        return MP_ENCODING_ERROR;
    }

    // add reference output, if there is one
    if (!receiverAddress.empty() && receiverAddress != feeAddress) {
        CScript scriptPubKey = GetScriptForDestination(DecodeDestination(receiverAddress));
        vecSend.push_back(std::make_pair(scriptPubKey, GetDustThreshold(scriptPubKey)));
    }

    // convert into recipients objects
    std::vector<CRecipient> vecRecipients;
    for (size_t i = 0; i < vecSend.size(); ++i) {
        const std::pair<CScript, int64_t>& vec = vecSend[i];
        CRecipient recipient = {vec.first, vec.second, false};
        vecRecipients.push_back(recipient);
    }

    bool fSuccess = false;
    CWalletTx wtxNew;
    CReserveKey reserveKey(pwalletMain);
    int64_t nFeeRequired = 0;
    std::string strFailReason;
    int nChangePosRet = 0; // add change first

    // set change
    CCoinControl coinControl;
    coinControl.destChange = DecodeDestination(feeAddress);
    coinControl.fAllowOtherInputs = true;

    if (!SelectAllCoins(senderAddress, coinControl)) {
        PrintToLog("%s: ERROR: sender %s has no coins\n", __func__, senderAddress);
        return MP_INPUTS_INVALID;
    }

    // prepare sources for fees
    std::set<CTxDestination> feeSources;
    feeSources.insert(DecodeDestination(feeAddress));

    std::vector<COutPoint> vLockedCoins;
    LockUnrelatedCoins(pwalletMain, feeSources, vLockedCoins);

    fSuccess = pwalletMain->CreateTransaction(vecRecipients, wtxNew, reserveKey, nFeeRequired, nChangePosRet, strFailReason, &coinControl, ALL_COINS, false);
    if (fSuccess && nChangePosRet == -1 && receiverAddress == feeAddress) {
        fSuccess = false;
        strFailReason = "send to self without change";
    }

    // to restore the original order of inputs, create a new transaction and add
    // inputs and outputs step by step
    CMutableTransaction tx;

    std::vector<COutPoint> vSelectedInputs;
    coinControl.ListSelected(vSelectedInputs);

    // add previously selected coins
    for (const COutPoint& txIn : vSelectedInputs) {
        tx.vin.push_back(CTxIn(txIn));
    }

    // add other selected coins
    for (const CTxIn& txin : wtxNew.vin) {
        if (!coinControl.IsSelected(txin.prevout)) {
            tx.vin.push_back(txin);
        }
    }

    // add outputs
    for (const CTxOut& txOut : wtxNew.vout) {
        tx.vout.push_back(txOut);
    }

    // restore original locking state
    UnlockCoins(pwalletMain, vLockedCoins);

    // lock selected outputs for this transaction // TODO: could be removed?
    if (fSuccess) {
        for (const CTxIn& txIn : tx.vin) {
            pwalletMain->LockCoin(txIn.prevout);
        }
    }

    if (!fSuccess) {
        PrintToLog("%s: ERROR: wallet transaction creation failed: %s\n", __func__, strFailReason);
        return MP_ERR_CREATE_TX;
    }

    // sign the transaction

    // fetch previous transactions (inputs):
    CCoinsView viewDummy;
    CCoinsViewCache view(&viewDummy);
    {
        LOCK(mempool.cs);
        CCoinsViewCache &viewChain = *pcoinsTip;
        CCoinsViewMemPool viewMempool(&viewChain, mempool);
        view.SetBackend(viewMempool); // temporarily switch cache backend to db+mempool view

        for (const CTxIn& txin : tx.vin) {
            const auto& prevout = txin.prevout;
            view.AccessCoin(prevout); // this is certainly allowed to fail
        }

        view.SetBackend(viewDummy); // switch back to avoid locking mempool for too long
    }

    int nHashType = SIGHASH_ALL;
    const CKeyStore& keystore = *pwalletMain;

    for (unsigned int i = 0; i < tx.vin.size(); i++) {
        CTxIn& txin = tx.vin[i];
        const Coin& coin = view.AccessCoin(txin.prevout);
        if (coin.IsSpent()) {
            PrintToLog("%s: ERROR: wallet transaction signing failed: input not found or already spent\n", __func__);
            continue;
        }

        const CScript& prevPubKey = coin.out.scriptPubKey;
        const CAmount& amount = coin.out.nValue;

        SignatureData sigdata;
        if (!ProduceSignature(MutableTransactionSignatureCreator(&keystore, &tx, i, amount, nHashType), prevPubKey, sigdata, false)) {
            PrintToLog("%s: ERROR: wallet transaction signing failed\n", __func__);
            return MP_ERR_CREATE_TX;
        }

        UpdateTransaction(tx, i, sigdata);
    }

    // send the transaction

    CValidationState state;

    if (!AcceptToMemoryPool(mempool, state, tx, false, NULL, false, DEFAULT_TRANSACTION_MAXFEE)) {
        PrintToLog("%s: ERROR: failed to broadcast transaction: %s\n", __func__, state.GetRejectReason());
        return MP_ERR_COMMIT_TX;
    }
    RelayTransaction(tx, *g_connman.get());

    retTxid = tx.GetHash();

    return 0;
#else
    return MP_ERR_WALLET_ACCESS;
#endif

}