/******************************************************************************
 * Copyright © 2014-2019 The SuperNET Developers.                             *
 *                                                                            *
 * See the AUTHORS, DEVELOPER-AGREEMENT and LICENSE files at                  *
 * the top-level directory of this distribution for the individual copyright  *
 * holder information and the developer policies on copyright and licensing.  *
 *                                                                            *
 * Unless otherwise agreed in a custom licensing agreement, no part of the    *
 * SuperNET software, including this file may be copied, modified, propagated *
 * or distributed except according to the terms contained in the LICENSE file *
 *                                                                            *
 * Removal or modification of this copyright notice is prohibited.            *
 *                                                                            *
 ******************************************************************************/

#include "CCtokens.h"
#include "CCassets.h"
#include "CCtokens_impl.h"
#include "CCassetsCore_impl.h"
#include "CCNFTData.h"

/*
 TODO: update:

 Assets can be created or transferred.
 
 native coins are also locked in the EVAL_ASSETS address, so we need a strict rule on when utxo in the special address are native coins and when they are assets. The specific rule that must not be violated is that vout0 for 'b'/'B' funcid are native coins. All other utxo locked in the special address are assets.
 
 To create an asset use CC EVAL_ASSETS to create a transaction where vout[0] funds the assets. Externally each satoshi can be interpreted to represent 1 asset, or 100 million satoshis for one asset with 8 decimals, and the other decimals in between. The interpretation of the number of decimals is left to the higher level usages.
 
 Once created, the assetid is the txid of the create transaction and using the assetid/0 it can spend the assets to however many outputs it creates. The restriction is that the last output must be an opreturn with the assetid. The sum of all but the first output needs to add up to the total assetoshis input. The first output is ignored and used for change from txfee.
 
 What this means is that vout 0 of the creation txid and vouts 1 ... n-2 for transfer vouts are assetoshi outputs.
 
 There is a special type of transfer to an unspendable address, that locks the asset and creates an offer for all. The details specified in the opreturn. In order to be compatible with the signing restrictions, the "unspendable" address is actually an address whose privkey is known to all. Funds sent to this address can only be spent if the swap parameters are fulfilled, or if the original pubkey cancels the offer by spending it.
 
 Types of transactions:
 create name:description -> txid for assetid
 transfer <pubkey> <assetid> -> [{address:amount}, ... ] // like withdraw api
 selloffer <pubkey> <txid/vout> <amount> -> cancel or fillsell -> mempool txid or rejected, might not confirm
 buyoffer <amount> <assetid> <required> -> cancelbuy or fillbuy -> mempool txid or rejected, might not confirm
 exchange <pubkey> <txid/vout> <required> <required assetid> -> cancel or fillexchange -> mempool txid or rejected, might not confirm
 
 assetsaddress <pubkey> // all assets end up in a special address for each pubkey
 assetbalance <pubkey> <assetid>
 assetutxos <pubkey> <assetid>
 assetsbalances <pubkey>
 asks <assetid>
 bids <assetid>
 swaps <assetid>
 
 valid CC output: create or transfer or buyoffer or selloffer or exchange or cancel or fill
 
 
 buyoffer:
 vins.*: normal inputs (bid + change)
 vout.0: amount of bid to unspendable
 vout.1: CC output for marker
 vout.2: normal output for change (if any)
 vout.n-1: opreturn [EVAL_ASSETS] ['b'] [assetid] [amount of asset required] [origpubkey]

 cancelbuy:
 vin.0: normal input
 vin.1: unspendable.(vout.0 from buyoffer) buyTx.vout[0]
 vin.2: CC marker from buyoffer for txfee
 vout.0: vin.1 value to original pubkey buyTx.vout[0].nValue -> [origpubkey]
 vout.1: vin.2 back to users pubkey
 vout.2: normal output for change (if any)
 vout.n-1: opreturn [EVAL_ASSETS] ['o'] [assetid] 0 0 [origpubkey]
 
 fillbuy:
 vin.0: normal input
 vin.1: unspendable.(vout.0 from buyoffer) buyTx.vout[0]
 vin.2+: valid CC output satisfies buyoffer (*tx.vin[2])->nValue
 vout.0: remaining amount of bid to unspendable
 vout.1: vin.1 value to signer of vin.2
 vout.2: vin.2 assetoshis to original pubkey
 vout.3: CC output for assetoshis change (if any)
 vout.4: normal output for change (if any)
 vout.n-1: opreturn [EVAL_ASSETS] ['B'] [assetid] [remaining asset required] [origpubkey]

 selloffer:
 vin.0: normal input
 vin.1+: valid CC output for sale
 vout.0: vin.1 assetoshis output to CC to unspendable
 vout.1: CC output for marker
 vout.2: CC output for change (if any)
 vout.3: normal output for change (if any)
 vout.n-1: opreturn [EVAL_ASSETS] ['s'] [assetid] [amount of native coin required] [origpubkey]
 
 exchange:
 vin.0: normal input
 vin.1+: valid CC output
 vout.0: vin.1 assetoshis output to CC to unspendable
 vout.1: CC output for change (if any)
 vout.2: normal output for change (if any)
 vout.n-1: opreturn [EVAL_ASSETS] ['e'] [assetid] [assetid2] [amount of asset2 required] [origpubkey]
 
 cancel:
 vin.0: normal input
 vin.1: unspendable.(vout.0 from exchange or selloffer) sellTx/exchangeTx.vout[0] inputTx
 vin.2: CC marker from selloffer for txfee
 vout.0: vin.1 assetoshis to original pubkey CC sellTx/exchangeTx.vout[0].nValue -> [origpubkey]
 vout.1: vin.2 back to users pubkey
 vout.2: normal output for change (if any)
 vout.n-1: opreturn [EVAL_ASSETS] ['x'] [assetid]
 
 fillsell:
 vin.0: normal input
 vin.1: unspendable.(vout.0 assetoshis from selloffer) sellTx.vout[0]
 vin.2+: normal output that satisfies selloffer (*tx.vin[2])->nValue
 vout.0: remaining assetoshis -> unspendable
 vout.1: vin.1 assetoshis to signer of vin.2 sellTx.vout[0].nValue -> any
 vout.2: vin.2 value to original pubkey [origpubkey]
 vout.3: CC asset for change (if any)
 vout.4: CC asset2 for change (if any) 'E' only
 vout.5: normal output for change (if any)
 vout.n-1: opreturn [EVAL_ASSETS] ['S'] [assetid] [amount of coin still required] [origpubkey]
 
 fillexchange:
 vin.0: normal input
 vin.1: unspendable.(vout.0 assetoshis from exchange) exchangeTx.vout[0]
 vin.2+: valid CC assetid2 output that satisfies exchange (*tx.vin[2])->nValue
 vout.0: remaining assetoshis -> unspendable
 vout.1: vin.1 assetoshis to signer of vin.2 exchangeTx.vout[0].nValue -> any
 vout.2: vin.2 assetoshis2 to original pubkey [origpubkey]
 vout.3: normal output for change (if any)
 vout.n-1: opreturn [EVAL_ASSETS] ['E'] [assetid vin0+1] [assetid vin2] [remaining asset2 required] [origpubkey]
*/

// checks cc vins (only mine) and cc vouts (all) count. If param is -1 then it does not check it
static bool CountCCVinVouts(struct CCcontract_info *cpAssets, const CTransaction & tx, int32_t _ccvins, int32_t _ccvouts)
{
    int32_t ccvins = 0;
    int32_t ccvouts = 0;

    if (_ccvins >= 0)   {
        for (auto const &vin : tx.vin)
            if (cpAssets->ismyvin(vin.scriptSig))
                ccvins ++;
    }
    if (_ccvouts >= 0)  {
        for (auto const &vout : tx.vout)
            if (vout.scriptPubKey.IsPayToCryptoCondition())
                ccvouts ++;
    }

    if (_ccvins >= 0 && _ccvins != ccvins || _ccvouts >= 0 && _ccvouts != ccvouts) {
        LOGSTREAMFN(ccassets_log, CCLOG_ERROR, stream << " invalid vins or vout count: ccvins=" << ccvins << " check=" << _ccvins << " ccvouts=" << ccvouts << " check=" << _ccvouts << std::endl);
        return false;
    }
    return true;
}


// tx validation
template<class T, class A>
static bool AssetsValidateInternal(struct CCcontract_info *cp, Eval* eval,const CTransaction &tx, uint32_t nIn)
{
    static uint256 zero;
    CTxDestination address; 
    CTransaction vinTx, createTx; 
    uint256 hashBlock, assetid, assetid2; 
	//int32_t preventCCvins, preventCCvouts; 
    int32_t ccvins = -1, ccvouts = -1;
	int64_t unit_price, vin_unit_price; 
    vuint8_t vorigpubkey, vin_origpubkey, vopretNonfungible;
    TokenDataTuple tokenData;
	uint8_t funcid, evalCodeInOpret; 
	char destaddr[KOMODO_ADDRESS_BUFSIZE], origNormalAddr[KOMODO_ADDRESS_BUFSIZE], ownerNormalAddr[KOMODO_ADDRESS_BUFSIZE]; 
    char origTokensCCaddr[KOMODO_ADDRESS_BUFSIZE], origCCaddrDummy[KOMODO_ADDRESS_BUFSIZE]; 
    char tokensDualEvalUnspendableCCaddr[KOMODO_ADDRESS_BUFSIZE], origAssetsCCaddr[KOMODO_ADDRESS_BUFSIZE];

    CAmount vin_tokens = 0;
    CAmount vin_assetoshis = 0;

    int32_t numvins = tx.vin.size();
    int32_t numvouts = tx.vout.size();
    //preventCCvins = preventCCvouts = -1;
        
	if (tx.vout.size() == 0)
		return eval->Invalid("AssetValidate: no vouts");

    if((funcid = A::DecodeAssetTokenOpRet(tx.vout[numvouts-1].scriptPubKey, evalCodeInOpret, assetid, assetid2, unit_price, vorigpubkey)) == 0 )
        return eval->Invalid("AssetValidate: invalid opreturn payload");

	// reinit cpAssets as we could set evalcodeNFT in it
	struct CCcontract_info *cpAssets, assetsC;
	cpAssets = CCinit(&assetsC, A::EvalCode());

	// we need this for validating single-eval tokens' vins/vous:
	struct CCcontract_info *cpTokens, tokensC;
	cpTokens = CCinit(&tokensC, T::EvalCode());

    // non-fungible tokens support:
    GetTokenData<T>(assetid, tokenData, vopretNonfungible);
    uint64_t royaltyFract = 0;  // royalty is N in N/1000 fraction
    if (vopretNonfungible.size() > 0)   {
        cpAssets->evalcodeNFT = vopretNonfungible.begin()[0];
        GetNftDataAsUint64(vopretNonfungible, NFTPROP_ROYALTY, royaltyFract);
        if (royaltyFract > NFTROYALTY_DIVISOR-1)
            royaltyFract = NFTROYALTY_DIVISOR-1; // royalty upper limit
    }

    vuint8_t ownerpubkey = std::get<0>(tokenData);
    Getscriptaddress(ownerNormalAddr, CScript() << ownerpubkey << OP_CHECKSIG);

	// find dual-eval tokens global addr where tokens are locked:
	GetTokensCCaddress(cpAssets, tokensDualEvalUnspendableCCaddr, GetUnspendable(cpAssets, NULL), A::IsMixed());

    // originator cc address, this is for marker validation:
    GetCCaddress(cpAssets, origAssetsCCaddr, vorigpubkey, A::IsMixed()); 

    if( IsCCInput(tx.vin[0].scriptSig) != false )   // vin0 should be normal vin
        return eval->Invalid("illegal asset cc vin0");
    else if( numvouts < 2 )
        return eval->Invalid("too few vouts");  // it was if(numvouts < 1) but it refers at least to vout[1] below
    
    if (assetid == zeroid)
        return eval->Invalid("illegal opreturn assetid");
    
    switch( funcid )
    {
        case 'c': 
            // see token cc
			return eval->Invalid("invalid asset funcid \'c\'");
            
        case 't': 
            // see token cc
			return eval->Invalid("invalid asset funcid \'t\'");
                        
        case 'b': // bid offer
            //vins.*: normal inputs (bid + change)
            //vout.0: amount of bid to cc assets global address
            //vout.1: CC output for marker
            //vout.2: normal output for change (if any)
            //vout.n-1: opreturn [EVAL_ASSETS] ['b'] [assetid] [amount of asset required] [origpubkey]

            // as we don't use tokenconvert 'b' does not have cc inputs and we should not be here:
            if (cpAssets->evalcode == EVAL_ASSETS)
                return eval->Invalid("invalid asset funcid (b)");
            else {
                // for EVAL_ASSETSV2 check the creation tx 
                char origTokenAddr[KOMODO_ADDRESS_BUFSIZE];
                
                //preventCCvouts = 2;
                if (numvouts < 3)
                    return eval->Invalid("too few vouts");
                else if( A::ConstrainVout(tx.vout[0], 1, cpAssets->unspendableCCaddr, 0LL, A::EvalCode()) == false )
                    return eval->Invalid("invalid funding for bid");
                else if( A::ConstrainVout(tx.vout[1], 1, origAssetsCCaddr, ASSETS_MARKER_AMOUNT, A::EvalCode()) == false )       // marker to originator asset cc addr
                    return eval->Invalid("invalid vout1 marker for original pubkey");
                else if( TotalPubkeyNormalInputs(tx, pubkey2pk(vorigpubkey)) == 0 ) // check tx is signed by originator pubkey
                    return eval->Invalid("not the originator pubkey signed");

                // check should not be assets cc vins:
                for (auto const &vin : tx.vin)   {
                    if (cpAssets->ismyvin(vin.scriptSig))
                        return eval->Invalid("could not have cc assets vin for creation tx");
                }
                ccvouts = 2;
            }            
            break;

        case 'o': // cancelbid
            //vin.0: normal input
            //vin.1: unspendable.(vout.0 from buyoffer) buyTx.vout[0]
            //vin.2: CC marker from buyoffer for txfee
            //vout.0: vin.1 value to original pubkey buyTx.vout[0].nValue -> [origpubkey]
            //vout.1: vin.2 back to users pubkey
            //vout.2: normal output for change (if any)
            //vout.n-1: opreturn [EVAL_ASSETS] ['o']
            ccvins = 2;  // order and marker
            ccvouts = 0;
            if (vin_assetoshis = AssetValidateBuyvin<A>(cpAssets, eval, vin_unit_price, vin_origpubkey, origCCaddrDummy, origNormalAddr, tx, assetid) == 0)
                return(false);
            
            if (tx.vout[0].nValue > ASSETS_NORMAL_DUST)  {
                if( A::ConstrainVout(tx.vout[0], 0, origNormalAddr, vin_assetoshis, 0) == false )
                    return eval->Invalid("invalid refund for cancelbid");
            } else {
                bool isCCVout = A::ConstrainVout(tx.vout[0], 1, cpAssets->unspendableCCaddr, vin_assetoshis, 0);
                if( A::ConstrainVout(tx.vout[0], 0, origNormalAddr, vin_assetoshis, 0) == false &&
                    !isCCVout )  // dust allowed to go back
                    return eval->Invalid("invalid refund for cancelbid");
                if( isCCVout )
                    ccvouts ++;
            }
            if( TotalPubkeyNormalInputs(tx, pubkey2pk(vin_origpubkey)) == 0 ) // check tx is signed by originator pubkey
                return eval->Invalid("not the originator pubkey signed");

            //preventCCvins = 2;
            //preventCCvouts = 0;
            //fprintf(stderr,"cancelbuy validated to origaddr.(%s)\n",origNormalAddr);
            break;
            
        case 'B': // fillbid:
            //vin.0: normal input
            //vin.1: cc assets unspendable (vout.0 from buyoffer) buyTx.vout[0]
            //vin.2+: valid CC output satisfies buyoffer (*tx.vin[2])->nValue
            //vout.0: remaining amount of bid to cc assets global address
            //vout.1: vin.1 value to signer of vin.2
            //vout.2: vin.2 assetoshis to original pubkey (-royalty)
            //vout.3: royalty to token owner (optional)
            //vout.4: CC output for assetoshis change (if any)
            //vout.5: normal output for change (if any)
            //vout.n-1: opreturn [EVAL_ASSETS] ['B'] [assetid] [remaining asset required] [origpubkey]
            //preventCCvouts = 4;
			ccvins = 1;
            ccvouts = 3;

            if( (vin_assetoshis = AssetValidateBuyvin<A>(cpAssets, eval, vin_unit_price, vin_origpubkey, origTokensCCaddr, origNormalAddr, tx, assetid)) == 0 )
                return false;
            else if( numvouts < 4 )
                return eval->Invalid("not enough vouts for fillbid");
            else if( vin_origpubkey != vorigpubkey )    // originator pk does not change in the new opret
                return eval->Invalid("mismatched opreturn originator pubkeys for fillbid");
            else if (unit_price != vin_unit_price)
                return eval->Invalid("mismatched unit price for fillbid");
            else
            {
                int32_t r = royaltyFract > 0 ? 1 : 0;
                CAmount assetoshis = tx.vout[0].nValue + tx.vout[1].nValue + (royaltyFract > 0 ? tx.vout[2].nValue : 0);
                if( vin_assetoshis != assetoshis )             // coins -> global cc address (remainder) + normal self address
                    return eval->Invalid("input cc value doesnt match vout0+1" + std::string(royaltyFract > 0 ? "+2" : "") + " for fillbid");
                
                vin_tokens = AssetsGetCCInputs(cpTokens, NULL, tx);
                if( tx.vout[4+r].scriptPubKey.IsPayToCryptoCondition() != false )    // if tokens remainder exists 
                {
                    if( A::ConstrainVout(tx.vout[2+r], 1, origTokensCCaddr, 0LL, T::EvalCode()) == false )	// tokens to originator cc addr (tokens+nonfungible evalcode)
                        return eval->Invalid("vout" + std::to_string(2+r) + " tokens value should go to originator pubkey for fillbid");
                    else if( vin_tokens != tx.vout[2+r].nValue + tx.vout[4+r].nValue )    // tokens from cc global address -> token global addr (remainder) + originator cc address
                        return eval->Invalid("tokens inputs doesnt match tokens vout" + std::to_string(2+r) + "+" + "vout"+std::to_string(4+r) + " for fillbid");
                    //preventCCvouts ++;
                    ccvouts++;
                }
                else if( A::ConstrainVout(tx.vout[2+r], 1, origTokensCCaddr, vin_tokens, T::EvalCode()) == false )   // all tokens to originator cc addr, no cc change present
                    return eval->Invalid("vout" + std::to_string(2+r) + " should have tokens to originator cc addr for fillbid");

                if( A::ConstrainVout(tx.vout[1], 0, NULL, 0LL, 0) == false )                            // amount paid for tokens goes to normal addr (we can't check 'self' address)
                    return eval->Invalid("vout1 should be normal for fillbid");
                else if( A::ConstrainVout(tx.vout[3+r], 1, origAssetsCCaddr, ASSETS_MARKER_AMOUNT, A::EvalCode()) == false )       // marker to originator asset cc addr
                    return eval->Invalid("invalid vout" + std::to_string(3+r) + " marker for originator pubkey for fillbid");
                
                CAmount received_value = royaltyFract > 0 ? tx.vout[1].nValue + tx.vout[2].nValue : tx.vout[1].nValue; // vout1 paid value to seller, vout2 royalty to owner
                CAmount paid_units = tx.vout[2+r].nValue;
                if( ValidateBidRemainder(unit_price, tx.vout[0].nValue, vin_assetoshis, received_value, paid_units) == false ) // check real price and coins spending from global addr
                    return eval->Invalid("vout" + std::to_string(2+r) + " mismatched remainder for fillbid");
                if (unit_price <= 0)
                    return eval->Invalid("invalid unit price");
                if (royaltyFract > 0) {
                    if ((tx.vout[1].nValue + tx.vout[2].nValue) / NFTROYALTY_DIVISOR * royaltyFract != tx.vout[2].nValue)  // validate royalty value
                        return eval->Invalid("vout2 invalid royalty amount for fillask");
                    if( A::ConstrainVout(tx.vout[2], 0, ownerNormalAddr, 0LL, 0) == false )        // validate owner royalty dest
                        return eval->Invalid("vout2 invalid royalty detination for fillask");
                }
                if( tx.vout[0].nValue / unit_price != 0 ) // remaining tokens to buy
                {
                    if( A::ConstrainVout(tx.vout[0], 1, cpAssets->unspendableCCaddr, 0LL, A::EvalCode()) == false )  // if remainder sufficient to buy tokens -> coins to asset global cc addr
                        return eval->Invalid("mismatched vout0 global assets CC addr for fillbid");
                }
                else 
                {   // remaining_units == 0
                    if( tx.vout[0].nValue > ASSETS_NORMAL_DUST )  {
                        if( A::ConstrainVout(tx.vout[0], 0, origNormalAddr, 0LL, 0) == false )  // remainder less than token price should return to originator normal addr
                            return eval->Invalid("vout0 should be originator normal address with remainder for fillbid");
                        ccvouts--;
                    }
                    else {
                        bool isCCVout = A::ConstrainVout(tx.vout[0], 1, cpAssets->unspendableCCaddr, 0, 0);
                        if( A::ConstrainVout(tx.vout[0], 0, origNormalAddr, 0LL, 0) == false &&
                            !isCCVout ) // dust allowed to go back to cc global addr
                            return eval->Invalid("vout0 should be originator normal address with remainder or dust should go back to global addr for fillbid");
                        if( !isCCVout )
                            ccvouts--;
                    }
                }
            }
            //fprintf(stderr,"fillbuy validated\n");
            break;
        //case 'e': // sell swap offer
        //    break; // disable swaps
        case 's': // ask offer
            //vin.0: normal input
            //vin.1+: valid CC output for sale
            //vout.0: vin.1 assetoshis output to CC assets global address
            //vout.1: CC output to CC assets global address for marker
            //vout.2: CC output for change (if any)
            //vout.3: normal output for change (if any)
            //'s'.vout.n-1: opreturn [EVAL_ASSETS] ['s'] [assetid] [amount of native coin required] [origpubkey]
            //'e'.vout.n-1: opreturn [EVAL_ASSETS] ['e'] [assetid] [assetid2] [amount of asset2 required] [origpubkey]

            // as we don't use tokenconvert 's' does not have cc inputs and we should not be here:
            if (cpAssets->evalcode == EVAL_ASSETS)
                return eval->Invalid("invalid asset funcid (s)");
            else {
                // for EVAL_ASSETSV2 check the creation tx 
                char origTokenAddr[KOMODO_ADDRESS_BUFSIZE];
                CPubKey origpk = pubkey2pk(vorigpubkey);

                //preventCCvouts = 2;
                ccvouts = 2;
                if (numvouts < 3)
                    return eval->Invalid("too few vouts");
                else if (A::ConstrainVout(tx.vout[0], 1, tokensDualEvalUnspendableCCaddr, 0LL, T::EvalCode()) == false)      // tokens sent to global addr
                    return eval->Invalid("invalid vout0 global two eval address for sell");
                else if( A::ConstrainVout(tx.vout[1], 1, origAssetsCCaddr, ASSETS_MARKER_AMOUNT, A::EvalCode()) == false )  // marker to originator asset cc addr
                    return eval->Invalid("invalid vout1 marker for originator pubkey");
                else if (TotalPubkeyNormalInputs(tx, origpk) == 0)  // check tx is signed by originator pubkey
                    return eval->Invalid("not the originator pubkey signed");

                //GetTokensCCaddress(cpTokens, origTokenAddr, origpk, A::IsMixed());
                if (tx.vout[2].scriptPubKey.IsPayToCryptoCondition())
                    if (!tx.vout[2].scriptPubKey.IsPayToCCV2() || tx.vout[2].scriptPubKey.SpkHasEvalcodeCCV2(T::EvalCode()))  // have token change
                        ccvouts ++; //    preventCCvouts ++;

                // check should not be assets cc vins:
                for (auto const &vin : tx.vin)   {
                    if (cpAssets->ismyvin(vin.scriptSig))
                        return eval->Invalid("could not have cc assets vin for creation tx");
                }
            }
            break;
            
        case 'x': // cancel ask            
            //vin.0: normal input
            //vin.1: cc assets global address (vout.0 from exchange or selloffer) sellTx/exchangeTx.vout[0] inputTx
            //vin.2: CC marker from selloffer for txfee
            //vout.0: vin.1 assetoshis to original pubkey CC sellTx/exchangeTx.vout[0].nValue -> [origpubkey]
            //vout.1: vin.2 back to users pubkey
            //vout.2: normal output for change (if any)
            //vout.n-1: opreturn [EVAL_ASSETS] ['x'] [assetid]

            if( (vin_tokens = AssetValidateSellvin<A>(cpAssets, eval, vin_unit_price, vin_origpubkey, origTokensCCaddr, origNormalAddr, tx, assetid)) == 0 )  
                return false;  // eval already set
            else if (A::ConstrainVout(tx.vout[0], 1, origTokensCCaddr, vin_tokens, T::EvalCode()) == false)      // tokens returning to originator cc addr
                return eval->Invalid("invalid vout0 for cancelask");
            else if (TotalPubkeyNormalInputs(tx, pubkey2pk(vin_origpubkey)) == 0)  // check tx is signed by originator pubkey
                return eval->Invalid("not the originator pubkey signed");
            //preventCCvins = 3;
            //preventCCvouts = 1;
            ccvins = 2;  // order and marker
            ccvouts = 1;
            break;
            
        case 'S': // fill ask
            //vin.0: normal input
            //vin.1: cc assets global address (vout.0 assetoshis from selloffer) sellTx.vout[0]
            //'S'.vin.2+: normal output that satisfies selloffer (*tx.vin[2])->nValue
            //vout.0: remaining assetoshis -> cc assets global address
            //vout.1: vin.1 assetoshis to signer of vin.2 sellTx.vout[0].nValue -> any
            //'S'.vout.2: vin.2 value to original pubkey [origpubkey]
            //vout.3: normal output for change (if any)
            //'S'.vout.n-1: opreturn [EVAL_ASSETS] ['S'] [assetid] [amount of coin still required] [origpubkey]
			ccvins = 1;
            ccvouts = 3;

            if( (vin_tokens = AssetValidateSellvin<A>(cpAssets, eval, vin_unit_price, vin_origpubkey, origTokensCCaddr, origNormalAddr, tx, assetid)) == 0 )
                return false;  // eval is set
            else if( numvouts < 4 )
                return eval->Invalid("not enough vouts for fillask");
            else if( vin_origpubkey != vorigpubkey )
                return eval->Invalid("mismatched origpubkeys for fillask");
            else if (unit_price != vin_unit_price)
                return eval->Invalid("mismatched unit price for fillask");
            else
            {
                int32_t markerVout = royaltyFract > 0 ? 4 : 3;
                if( vin_tokens != tx.vout[0].nValue + tx.vout[1].nValue )
                    return eval->Invalid("locked value doesnt match vout0+1 fillask");

                CAmount paid_value = royaltyFract > 0 ? tx.vout[2].nValue+tx.vout[3].nValue : tx.vout[2].nValue; // vout2 paid value to seller, vout3 royalty to owner
                if( ValidateAskRemainder(unit_price, tx.vout[0].nValue, vin_tokens, tx.vout[1].nValue, paid_value) == false ) 
                    return eval->Invalid("mismatched vout0 remainder for fillask");
                else if( A::ConstrainVout(tx.vout[1], 1, NULL, 0LL, T::EvalCode()) == false )      // do not check tokens buyer's 'self' cc addr
                    return eval->Invalid("vout1 should be cc for fillask");
                else if( A::ConstrainVout(tx.vout[2], 0, origNormalAddr, 0LL, 0) == false )        // coins to originator normal addr
                    return eval->Invalid("vout2 should be cc for fillask");
                if (royaltyFract > 0)  {
                    if ((tx.vout[2].nValue + tx.vout[3].nValue) / NFTROYALTY_DIVISOR * royaltyFract != tx.vout[3].nValue)  // validate royalty value
                        return eval->Invalid("vout3 invalid royalty amount for fillask");
                    if( A::ConstrainVout(tx.vout[3], 0, ownerNormalAddr, 0LL, 0) == false )        // validate owner royalty dest
                        return eval->Invalid("vout3 invalid royalty detination for fillask");
                }
                if( A::ConstrainVout(tx.vout[markerVout], 1, origAssetsCCaddr, ASSETS_MARKER_AMOUNT, A::EvalCode()) == false )  // marker to originator asset cc addr
                    return eval->Invalid("invalid marker vout for original pubkey");
                else // if( remaining_units != 0 )  // remaining expected amount in coins, should be anyway
                {
                    if( A::ConstrainVout(tx.vout[0], 1, tokensDualEvalUnspendableCCaddr, 0LL, A::EvalCode()) == false )   // tokens remainder on global addr
                        return eval->Invalid("mismatched vout0 two eval global CC addr for fillask");
                }
            }
            
            //fprintf(stderr,"fill validated\n");
            break;
        case 'E': // fillexchange	
			////////// not implemented yet ////////////
            return eval->Invalid("unexpected assets fillexchange funcid");
            break; // disable asset swaps
            //vin.0: normal input
            //vin.1: unspendable.(vout.0 assetoshis from selloffer) sellTx.vout[0]
            //vin.2+: valid CC assetid2 output that satisfies exchange (*tx.vin[2])->nValue
            //vout.0: remaining assetoshis -> unspendable
            //vout.1: vin.1 assetoshis to signer of vin.2 sellTx.vout[0].nValue -> any
            //vout.2: vin.2+ assetoshis2 to original pubkey [origpubkey]
            //vout.3: CC output for asset2 change (if any)
            //vout.3/4: normal output for change (if any)
            //vout.n-1: opreturn [EVAL_ASSETS] ['E'] [assetid vin0+1] [assetid vin2] [remaining asset2 required] [origpubkey]
            
			//if ( AssetExactAmounts(false, cp,inputs,outputs,eval,tx,assetid2) == false )    
            //    eval->Invalid("asset2 inputs != outputs");

			////////// not implemented yet ////////////
            if( (vin_tokens = AssetValidateSellvin<A>(cpTokens, eval, vin_unit_price, vin_origpubkey, origTokensCCaddr, origNormalAddr, tx, assetid)) == 0 )
                return(false);
            else if( numvouts < 3 )
                return eval->Invalid("not enough vouts for fillex");
            else if( vin_origpubkey != vorigpubkey )
                return eval->Invalid("mismatched origpubkeys for fillex");
            else if (unit_price != vin_unit_price)
                return eval->Invalid("mismatched unit price for fillex");
            else
            {
                /*
                if( tokens != tx.vout[0].nValue + tx.vout[1].nValue )
                    return eval->Invalid("locked value doesnt match vout0+1 fillex");
                else if( tx.vout[3].scriptPubKey.IsPayToCryptoCondition() != false )
				////////// not implemented yet ////////////
                {
                    if( ConstrainVout(tx.vout[2], 1, origTokensCCaddr, 0) == false )
                        return eval->Invalid("vout2 doesnt go to origpubkey fillex");
                    else if( tokens != tx.vout[2].nValue + tx.vout[3].nValue )
                    {
                        fprintf(stderr,"inputs %.8f != %.8f + %.8f\n",(double)tokens/COIN,(double)tx.vout[2].nValue/COIN,(double)tx.vout[3].nValue/COIN);
                        return eval->Invalid("asset inputs doesnt match vout2+3 fillex");
                    }
                }
				////////// not implemented yet ////////////
                else if( ConstrainVout(tx.vout[2], 1, origTokensCCaddr, tokens) == false )  
                    return eval->Invalid("vout2 doesnt match inputs fillex");
                else if( ConstrainVout(tx.vout[1], 0, 0, 0) == false )
                    return eval->Invalid("vout1 is CC for fillex");
                fprintf(stderr,"assets vout0 %lld, tokens %lld, vout2 %lld -> orig, vout1 %lld, total %lld\n", (long long)tx.vout[0].nValue, (long long)tokens,(long long)tx.vout[2].nValue,(long long)tx.vout[1].nValue,(long long)orig_units);
                if( ValidateSwapRemainder(remaining_units, tx.vout[0].nValue, tokens, tx.vout[1].nValue, tx.vout[2].nValue, orig_units) == false )
                //    return eval->Invalid("mismatched remainder for fillex");
                else if( ConstrainVout(tx.vout[1], 1, 0, 0) == false )
				////////// not implemented yet ////////////
                    return eval->Invalid("normal vout1 for fillex");
                else if( remaining_units != 0 ) 
                {
                    if( ConstrainVout(tx.vout[0], 1, (char *)cpAssets->unspendableCCaddr, 0) == false )  // TODO: unsure about this, but this is not impl yet anyway
                        return eval->Invalid("mismatched vout0 AssetsCCaddr for fillex");
                }
                */
            }
			////////// not implemented yet ////////////
            //fprintf(stderr,"fill validated\n");
            break;

        default:
            fprintf(stderr,"illegal assets funcid.(%c)\n",funcid);
            return eval->Invalid("unexpected assets funcid");
            //break;
    }

	//bool bPrevent = PreventCC(eval, tx, preventCCvins, numvins, preventCCvouts, numvouts);  // prevent presence of unknown cc vin or cc vouts in the tx
	//std::cerr << "AssetsValidate() PreventCC returned=" << bPrevent << std::endl;
	//return (bPrevent);
    // replaced PreventCC with min/max cc vin/vout calc to allow vin position flexibility:
    if (!CountCCVinVouts(cpAssets, tx, ccvins, ccvouts))
        return eval->Invalid("invalid cc vin or vout count");
    return true;
}

// redirect to AssetsValidateInternal and log error
bool AssetsValidate(struct CCcontract_info *cpAssets, Eval* eval,const CTransaction &tx, uint32_t nIn)
{
    // add specific chains exceptions for old token support:
    if (strcmp(ASSETCHAINS_SYMBOL, "SEC") == 0 && chainActive.Height() <= 144073)
        return true;
    
    if (strcmp(ASSETCHAINS_SYMBOL, "MGNX") == 0 && chainActive.Height() <= 210190)
        return true;

    if (!TokensIsVer1Active(NULL))   {	 
        bool valid = tokensv0::AssetsValidate(cpAssets, eval, tx, nIn); // call assets validation version 0
        if (!valid) 
            LOGSTREAMFN(ccassets_log, CCLOG_ERROR, stream << "v0 validation error: " << eval->state.GetRejectReason() << ", code: " << eval->state.GetRejectCode() << ", tx: " << HexStr(E_MARSHAL(ss << tx)) << std::endl);
        return valid;
    }

    if (!AssetsValidateInternal<TokensV1, AssetsV1>(cpAssets, eval, tx, nIn))    {
        LOGSTREAMFN(ccassets_log, CCLOG_ERROR, stream << "validation error: " << eval->state.GetRejectReason() << ", code: " << eval->state.GetRejectCode() << ", tx: " << HexStr(E_MARSHAL(ss << tx)) << std::endl);
        return false;
    }
    return true;
}

// redirect to AssetsValidateInternal and log error
bool Assetsv2Validate(struct CCcontract_info *cp, Eval* eval,const CTransaction &tx, uint32_t nIn)
{
    if (!AssetsValidateInternal<TokensV2, AssetsV2>(cp, eval, tx, nIn))    {
        LOGSTREAMFN(ccassets_log, CCLOG_ERROR, stream << "validation error: " << eval->state.GetRejectReason() << ", code: " << eval->state.GetRejectCode() << ", tx: " << HexStr(E_MARSHAL(ss << tx)) << std::endl);
        return false;
    }
    return true;
}