//===-------------------------- vSSA.cpp ----------------------------------===//
//
//					 The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
// Copyright (C) 2011-2012, 2014-2015	Victor Hugo Sperle Campos
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "vssa"

#include "vSSA.h"

using namespace llvm;

static const std::string vSSA_PHI = "vSSA_phi";
static const std::string vSSA_SIG = "vSSA_sigma";

STATISTIC(numsigmas, "Number of sigmas");
STATISTIC(numphis, "Number of phis");

void vSSA::getAnalysisUsage(AnalysisUsage &AU) const {
	AU.addRequired<DominanceFrontier>();
	AU.addRequired<DominatorTreeWrapperPass>();
}

bool vSSA::runOnFunction(Function &F) {
	// For some reason, in the DominatorTree pass, an unreachable BasicBlock
	// inside a function is considered to be dominated by anything. This goes
	// against the definition of dominance in my algorithm, and breaks for
	// some programs. Therefore, I decided to remove unreachable blocks from
	// the program before the conversion to e-SSA takes place.
	removeUnreachableBlocks(F);
	
	DTw_ = &getAnalysis<DominatorTreeWrapperPass>();
	DT_ = &DTw_->getDomTree();
	DF_ = &getAnalysis<DominanceFrontier>();
	
	// Iterate over all Basic Blocks of the Function, calling the function that creates sigma functions, if needed
	for (Function::iterator Fit = F.begin(), Fend = F.end(); Fit != Fend; ++Fit) {
		createSigmasIfNeeded(&*Fit);
	}
	return true;
}

void vSSA::createSigmasIfNeeded(BasicBlock *BB)
{
	TerminatorInst *ti = BB->getTerminator();
	// If the condition used in the terminator instruction is a Comparison instruction:
	//for each operand of the CmpInst, create sigmas, depending on some conditions
	/*
	if(isa<BranchInst>(ti)){
		BranchInst * bc = cast<BranchInst>(ti);
		if(bc->isConditional()){
			Value * cond = bc->getCondition();
			CmpInst *comparison = dyn_cast<CmpInst>(cond);
			for (User::const_op_iterator it = comparison->op_begin(), e = comparison->op_end(); it != e; ++it) {
				Value *operand = *it;
				if (isa<Instruction>(operand) || isa<Argument>(operand)) {
					insertSigmas(ti, operand);
				}
			}
		}
	}
	*/
	
	// CASE 1: Branch Instruction
	BranchInst *bi = NULL;
	SwitchInst *si = NULL;
	if ((bi = dyn_cast<BranchInst>(ti))) {
		if (bi->isConditional()) {
			Value *condition = bi->getCondition();
			
			ICmpInst *comparison = dyn_cast<ICmpInst>(condition);
			
			if (comparison) {
				// Create sigmas for ICmp operands
				for (User::const_op_iterator opit = comparison->op_begin(), opend = comparison->op_end(); opit != opend; ++opit) {
					Value *operand = *opit;
					
					if (isa<Instruction>(operand) || isa<Argument>(operand)) {
						insertSigmas(ti, operand);
						
						// If the operand is a result of a indirect instruction (e.g. ZExt, SExt, Trunc),
						// Create sigmas for the operands of the operands too
						CastInst *cinst = NULL;
						if ((cinst = dyn_cast<CastInst>(operand))) {
							if (isa<Instruction>(cinst->getOperand(0)) || isa<Argument>(cinst->getOperand(0))) {
								insertSigmas(ti, cinst->getOperand(0));
							}
						}
					}
				}
			}
		}
	}
	// CASE 2: Switch Instruction
	else if ((si = dyn_cast<SwitchInst>(ti))) {
		Value *condition = si->getCondition();
		
		if (isa<Instruction>(condition) || isa<Argument>(condition)) {
			insertSigmas(ti, condition);
			
			// If the operand is a result of a indirect instruction (e.g. ZExt, SExt, Trunc),
			// Create sigmas for the operands of the operands too
			CastInst *cinst = NULL;
			if ((cinst = dyn_cast<CastInst>(condition))) {
				if (isa<Instruction>(cinst->getOperand(0)) || isa<Argument>(cinst->getOperand(0))) {
					insertSigmas(ti, cinst->getOperand(0));
				}
			}
		}
	}
	
}

/*
 *  Insert sigmas for the value V;
 *  When sigmas are created, the creation of phis may happen too.
 */
void vSSA::insertSigmas(TerminatorInst *TI, Value *V)
{
	// Basic Block of the Terminator Instruction
	BasicBlock *BB = TI->getParent();
	
	// Vector that contains all vSSA_PHI nodes created in the process of creating sigmas for V
	SmallVector<PHINode*, 25> vssaphi_created;
	
	bool firstSigma = true;
	
	// Iterate over all successors of BB, checking if a sigma is needed
	for (unsigned i = 0, e = TI->getNumSuccessors(); i < e; ++i) {
		
		// Next Basic Block
		BasicBlock *BB_next = TI->getSuccessor(i);

		// If the successor is not BB itself and BB dominates the successor
		if (BB_next != BB && BB_next->getSinglePredecessor() != NULL && dominateOrHasInFrontier(BB, BB_next, V)) {
			// Create the sigma function (but before, verify if there is already an identical sigma function)
			if (verifySigmaExistance(V, BB_next, BB))
				continue;
			
			PHINode *sigma = PHINode::Create(V->getType(), 1, Twine(vSSA_SIG), &(BB_next->front()));
			sigma->addIncoming(V, BB);
			
			++numsigmas;
			
			// Rename uses of V to sigma(V)
			renameUsesToSigma(V, sigma);
			
			// Get the dominance frontier of the successor
			DominanceFrontier::iterator DF_BB = DF_->find(BB_next);
			
			
			/*
			 * Creation (or operand insertion) of the SSI_PHI
			 */
		
			// If BB_next has no dominance frontier, there is no need of SSI_PHI
			if (DF_BB == DF_->end())
				continue; 
			
			// If it was the first sigma created for V, phis may be needed, so we verify and create them
			if (firstSigma) {
				// Insert vSSA_phi, if necessary
				vssaphi_created = insertPhisForSigma(V, sigma);
			}
			else {
				// If it was not the first sigma created for V, then the phis already have been created and are stored in the deque, so we need only to insert the sigma as operand of them
				insertSigmaAsOperandOfPhis(vssaphi_created, sigma);
			}
		}
		
		// If phis were created, no more phis will be created, only operands will be inserted in them
		if (firstSigma && !vssaphi_created.empty())
			firstSigma = false;
	}
	
	// Populate the phi functions created
	populatePhis(vssaphi_created, V);
	
	// Insertion of vSSA_phi functions may require the creation of vSSA_phi functions too, so we call this function recursively
	for (SmallVectorImpl<PHINode*>::iterator vit = vssaphi_created.begin(), vend = vssaphi_created.end(); vit != vend; ++vit) {
		// Rename uses of V to phi
		renameUsesToPhi(V, *vit);
		
		insertPhisForPhi(V, *vit);
	}
}

/*
 * Renaming uses of V to uses of sigma
 * The rule of renaming is:
 *   - All uses of V in the dominator tree of sigma(V) are renamed, except for the sigma itself, of course
 *   - Uses of V in the dominance frontier of sigma(V) are renamed iff they are in PHI nodes (maybe this always happens)
 */
void vSSA::renameUsesToSigma(Value *V, PHINode *sigma)
{
	BasicBlock *BB_next = sigma->getParent();

	// Get the dominance frontier of the successor
	DominanceFrontier::iterator DF_BB = DF_->find(BB_next);
	
	// This vector of Instruction* points to the uses of V.
	// This auxiliary vector of pointers is used because the use_iterators are invalidated when we do the renaming
	SmallVector<Instruction*, 25> usepointers;
	unsigned i = 0, n = V->getNumUses();
	usepointers.resize(n);
	
	for (Value::user_iterator uit = V->user_begin(), uend = V->user_end(); uit != uend; ++uit, ++i)
		usepointers[i] = dyn_cast<Instruction>(*uit);
	
	for (i = 0; i < n; ++i) {
		if (usepointers[i] ==  NULL) {
			continue;
		}
		if (usepointers[i] == sigma) {
			continue;
		}
		if (isa<GetElementPtrInst>(usepointers[i])) {
			continue;
		}
		
		BasicBlock *BB_user = usepointers[i]->getParent();
		
		// Check if the use is in the dominator tree of sigma(V)
		if (DT_->dominates(BB_next, BB_user)){
			usepointers[i]->replaceUsesOfWith(V, sigma);
		}
		// Check if the use is in the dominance frontier of sigma(V)
		else if ((DF_BB != DF_->end()) && (DF_BB->second.find(BB_user) != DF_BB->second.end())) {
			// Check if the user is a PHI node (it has to be, but only for precaution)
			if (PHINode *phi = dyn_cast<PHINode>(usepointers[i])) {
				for (unsigned i = 0, e = phi->getNumIncomingValues(); i < e; ++i) {
					Value *operand = phi->getIncomingValue(i);
					
					if (operand != V)
						continue;
					
					if (DT_->dominates(BB_next, phi->getIncomingBlock(i))) {
						phi->setIncomingValue(i, sigma);
					}
				}
			}
		}
	}
}

/*
 *  Create phi functions in the dominance frontier of sigma(V), depending on some conditions
 */
SmallVector<PHINode*, 25> vSSA::insertPhisForSigma(Value *V, PHINode *sigma)
{
	SmallVector<PHINode*, 25> phiscreated;
	
	BasicBlock *BB = sigma->getParent();
	
	// Get the dominance frontier of the successor
	DominanceFrontier::iterator DF_BB = DF_->find(BB);
	
	// Iterate over the dominance frontier of the successor.
	// We have to create SSI_PHI functions in the basicblocks in the frontier, depending on some conditions
	for (std::set<BasicBlock*>::iterator DFit = DF_BB->second.begin(), DFend = DF_BB->second.end(); DFit != DFend; ++DFit) {
		BasicBlock *BB_infrontier = *DFit;
	
		// Check if the Value sigmed dominates this basicblock
		// We need to differentiate Instruction and Argument
		bool condition = false;
	
		if (Instruction *I = dyn_cast<Instruction>(V)) {
			condition = DT_->dominates(I->getParent(), BB_infrontier) && dominateAny(BB_infrontier, V);
		}
		else if (isa<Argument>(V)) {
			condition = dominateAny(BB_infrontier, V);
		}
	
		// If the original Value, for which the sigma was created, dominates the basicblock in the frontier, and this BB dominates any use of the Value, a vSSA_PHI is needed
		if (condition) {
	
//			// Need to discover from which BasicBlock the sigma value comes from
//			// To do that, we need to find which predecessor of BB_infrontier is dominated by BB_next
//			BasicBlock *predBB = NULL;
//		
//			for (pred_iterator PI = pred_begin(BB_infrontier), PE = pred_end(BB_infrontier); PI != PE; ++PI) {
//				predBB = *PI;
//			
//				if (DT_->dominates(BB, predBB)) {
//					break;
//				}
//			}
			
			// Create the vSSA_PHI, and put the phi node in the deques
			//NumReservedValues is a hint for the number of incoming edges that this phi node will have (use 0 if you really have no idea).
			PHINode *vssaphi = PHINode::Create(V->getType(), 0, Twine(vSSA_PHI), &(BB_infrontier->front()));
		
			phiscreated.push_back(vssaphi);
			
			++numphis;
		}
	}
	
	insertSigmaAsOperandOfPhis(phiscreated, sigma);
	
	return phiscreated;
}

void vSSA::insertPhisForPhi(Value *V, PHINode *phi)
{
	SmallVector<PHINode*, 25> phiscreated;
	
	BasicBlock *BB = phi->getParent();
	
	// Get the dominance frontier of the successor
	DominanceFrontier::iterator DF_BB = DF_->find(BB);
	
	// Iterate over the dominance frontier of the successor.
	// We have to create SSI_PHI functions in the basicblocks in the frontier, depending on some conditions
	for (std::set<BasicBlock*>::iterator DFit = DF_BB->second.begin(), DFend = DF_BB->second.end(); DFit != DFend; ++DFit) {
		BasicBlock *BB_infrontier = *DFit;
	
		// Check if the Value phied dominates this basicblock
		// We need to differentiate Instruction and Argument
		bool condition = false;
	
		if (Instruction *I = dyn_cast<Instruction>(V)) {
			condition = DT_->dominates(I->getParent(), BB_infrontier) && dominateAny(BB_infrontier, V);
		}
		else if (isa<Argument>(V)) {
			condition = dominateAny(BB_infrontier, V);
		}
	
		// If the original Value, for which the phi was created, dominates the basicblock in the frontier, and this BB dominates any use of the Value, a vSSA_PHI is needed
		if (condition) {
	
//			// Need to discover from which BasicBlock the sigma value comes from
//			// To do that, we need to find which predecessor of BB_infrontier is dominated by BB_next
//			BasicBlock *predBB = NULL;
//		
//			for (pred_iterator PI = pred_begin(BB_infrontier), PE = pred_end(BB_infrontier); PI != PE; ++PI) {
//				predBB = *PI;
//			
//				if (DT_->dominates(BB, predBB)) {
//					break;
//				}
//			}
			
			// Create the vSSA_PHI, and put the phi node in the deques
			PHINode *vssaphi = PHINode::Create(V->getType(), 0, Twine(vSSA_PHI), &(BB_infrontier->front()));
		
			phiscreated.push_back(vssaphi);
			
			++numphis;
			
			insertSigmaAsOperandOfPhis(phiscreated, phi);
			populatePhis(phiscreated, V);
			
			phiscreated.clear();
			
			// Rename uses of V to phi
			renameUsesToPhi(V, vssaphi);
			
			// Insertion of vSSA_phi functions may require the creation of vSSA_phi functions too, so we call this function recursively
			insertPhisForPhi(V, vssaphi);
		}
	}
}

/*
 * Renaming uses of V to uses of vSSA_PHI
 * The rule of renaming is:
 *   - All uses of V in the dominator tree of vSSA_PHI are renamed
 *   - Uses of V in the same basicblock of vSSA_PHI are only renamed if they are not in PHI functions
 *   - Uses of V in the dominance frontier follow the same rules of sigma renaming
 */
void vSSA::renameUsesToPhi(Value *V, PHINode *phi)
{
	// This vector of Instruction* points to the uses of operand.
	// This auxiliary vector of pointers is used because the use_iterators are invalidated when we do the renaming
	SmallVector<Instruction*, 25> usepointers;
	unsigned i = 0, n = V->getNumUses();
	 
	usepointers.resize(n);
	
	// This vector contains pointers to all sigmas that have its operand renamed to vSSA_phi
	// For them, we need to try to create phi functions again
	SmallVector<PHINode*, 25> sigmasRenamed;
	
	BasicBlock *BB_next = phi->getParent();
	
	// Get the dominance frontier of the successor
	DominanceFrontier::iterator DF_BB = DF_->find(BB_next);
	
	for (Value::user_iterator uit = V->user_begin(), uend = V->user_end(); uit != uend; ++uit, ++i)
		usepointers[i] = dyn_cast<Instruction>(*uit);
	
	BasicBlock *BB_parent = phi->getParent();
	 
	for (i = 0; i < n; ++i) {
		// Check if the use is in the dominator tree of vSSA_PHI
		if (DT_->dominates(BB_parent, usepointers[i]->getParent())) {
			if (BB_parent != usepointers[i]->getParent()) {
				usepointers[i]->replaceUsesOfWith(V, phi);
				
				// If this use is in a sigma, we need to check whether phis creation are needed again for this sigma
				if (PHINode *sigma = dyn_cast<PHINode>(usepointers[i])) {
					if (sigma->getName().startswith(vSSA_SIG)) {
						sigmasRenamed.push_back(sigma);
					}
				}
			}
			else if (!isa<PHINode>(usepointers[i]))
				usepointers[i]->replaceUsesOfWith(V, phi);
		}
		// Check if the use is in the dominance frontier of phi
	 	else if (DF_BB->second.find(usepointers[i]->getParent()) != DF_BB->second.end()) {
	 		// Check if the user is a PHI node (it has to be, but only for precaution)
	 		if (PHINode *phiuser = dyn_cast<PHINode>(usepointers[i])) {
	 			for (unsigned i = 0, e = phiuser->getNumIncomingValues(); i < e; ++i) {
	 				Value *operand = phiuser->getIncomingValue(i);
	 				
	 				if (operand != V)
	 					continue;
	 				
	 				if (DT_->dominates(BB_next, phiuser->getIncomingBlock(i))) {
	 					phiuser->setIncomingValue(i, phi);
	 				}
	 			}
	 		}
	 	}
	}
	
	for (unsigned k = 0, f = phi->getNumIncomingValues(); k < f; ++k) {
		PHINode *p = dyn_cast<PHINode>(phi->getIncomingValue(k));
		
		if (!p || !p->getName().startswith(vSSA_SIG))
			continue;
		
		Value *V = p->getIncomingValue(0);
		
		n = V->getNumUses();
		usepointers.resize(n);
		
		unsigned i = 0;
		
		for (Value::user_iterator uit = V->user_begin(), uend = V->user_end(); uit != uend; ++uit, ++i)
			usepointers[i] = dyn_cast<Instruction>(*uit);
		 
		for (i = 0; i < n; ++i) {
			// Check if the use is in the dominator tree of vSSA_PHI
			if (DT_->dominates(BB_parent, usepointers[i]->getParent())) {
				if (BB_parent != usepointers[i]->getParent()) {
					usepointers[i]->replaceUsesOfWith(V, phi);
				
					// If this use is in a sigma, we need to check whether phis creation are needed again for this sigma
					if (PHINode *sigma = dyn_cast<PHINode>(usepointers[i])) {
						if (sigma->getName().startswith(vSSA_SIG)) {
							sigmasRenamed.push_back(sigma);
						}
					}
				}
				else if (!isa<PHINode>(usepointers[i]))
					usepointers[i]->replaceUsesOfWith(V, phi);
			}
			// Check if the use is in the dominance frontier of phi
		 	else if (DF_BB->second.find(usepointers[i]->getParent()) != DF_BB->second.end()) {
		 		// Check if the user is a PHI node (it has to be, but only for precaution)
		 		if (PHINode *phiuser = dyn_cast<PHINode>(usepointers[i])) {
		 			for (unsigned i = 0, e = phiuser->getNumIncomingValues(); i < e; ++i) {
		 				Value *operand = phiuser->getIncomingValue(i);
		 				
		 				if (operand != V)
		 					continue;
		 				
		 				if (DT_->dominates(BB_next, phiuser->getIncomingBlock(i))) {
		 					phiuser->setIncomingValue(i, phi);
		 				}
		 			}
		 		}
		 	}
		}
		
	}
	
	// Try to create phis again for the sigmas whose operand was renamed to vSSA_phi
	// Also, renaming may need to be done again
	for (SmallVectorImpl<PHINode*>::iterator vit = sigmasRenamed.begin(), vend = sigmasRenamed.end(); vit != vend; ++vit) {
		renameUsesToSigma(phi, *vit);
		SmallVector<PHINode*, 25> vssaphis_created = insertPhisForSigma(phi, *vit);
		//insertSigmaAsOperandOfPhis(vssaphis_created, *vit);
		populatePhis(vssaphis_created, (*vit)->getIncomingValue(0));
	}
	
	// Creation of phis may require the creation of sigmas that were not created previosly, so we do it now
	//createSigmasIfNeeded(phi->getParent());
}

/*
 *  Insert the sigma as an operand of the vSSA_phis contained in the vector
 */
void vSSA::insertSigmaAsOperandOfPhis(SmallVector<PHINode*, 25> &vssaphi_created, PHINode *sigma)
{
	BasicBlock *BB = sigma->getParent();
	
	for (SmallVectorImpl<PHINode*>::iterator vit = vssaphi_created.begin(), vend = vssaphi_created.end(); vit != vend; ++vit) {
		PHINode *vssaphi = *vit;
		
		BasicBlock *predBB = NULL;
		pred_iterator PI = pred_begin(vssaphi->getParent());
		pred_iterator PE = pred_end(vssaphi->getParent());
		
		for (; PI != PE; ++PI) {
			predBB = *PI;
		
			if (DT_->dominates(BB, predBB)/* && (vssaphi->getBasicBlockIndex(predBB) == -1)*/) {
				vssaphi->addIncoming(sigma, predBB);
			}
		}		
	}
}

/*
 *  When phis are created, only the sigma and phi operands are inserted into them. Thus, we need to insert V, for which sigmas and phis were created, as incoming value of all
 *  incoming edges that still haven't an operand associated for them
 */
void vSSA::populatePhis(SmallVector<PHINode*, 25> &vssaphi_created, Value *V)
{
	// If any vSSA_PHI was created, iterate over the predecessors of vSSA_PHIs to insert V as an operand from the branches where sigma was not created
	for (SmallVectorImpl<PHINode*>::iterator vit = vssaphi_created.begin(), vend = vssaphi_created.end(); vit != vend; ++vit) {
		PHINode *vssaphi = *vit;
		BasicBlock *BB_parent = vssaphi->getParent();
		
		DenseMap<BasicBlock*, unsigned> howManyTimesIsPred;
		
		// Get how many times each basicblock is predecessor of BB_parent
		for (pred_iterator PI = pred_begin(BB_parent), PE = pred_end(BB_parent); PI != PE; ++PI) {
			BasicBlock *predBB = *PI;
			
			DenseMap<BasicBlock*, unsigned>::iterator mit = howManyTimesIsPred.find(predBB);
			
			if (mit == howManyTimesIsPred.end()) {
				howManyTimesIsPred.insert(std::make_pair(predBB, 1));
			}
			else {
				++mit->second;
			}
		}
		
		unsigned i, e;
		
		// If a predecessor already has incoming values in the vSSA_phi, we don't count them
		for (i = 0, e = vssaphi->getNumIncomingValues(); i < e; ++i) {
			--howManyTimesIsPred[vssaphi->getIncomingBlock(i)];
		}
		
		// Finally, add V as incoming value of predBB as many as necessary
		for (DenseMap<BasicBlock*, unsigned>::iterator mit = howManyTimesIsPred.begin(), mend = howManyTimesIsPred.end(); mit != mend; ++mit) {
			unsigned count;
			BasicBlock *predBB = mit->first;
			
			for (count = mit->second; count > 0; --count) {
				vssaphi->addIncoming(V, predBB);
			}
		}
		
		howManyTimesIsPred.clear();
	}
}

/// Test if the BasicBlock BB dominates any use or definition of value.
/// If it dominates a phi instruction that is on the same BasicBlock,
/// that does not count.
///
bool vSSA::dominateAny(BasicBlock *BB, Value *value) {
	// If the BasicBlock in-frontier is the same where the Value is defined, we don't create vSSA_PHI
	if (Instruction *I = dyn_cast<Instruction>(value)) {
		if (BB == I->getParent())
			return false;
	}
	else if (Argument *A = dyn_cast<Argument>(value)) {
		if (BB == &A->getParent()->getEntryBlock())
			return false;
	}
	
	// Iterate over the uses of Value.
	// If the use is in the BasicBlock in-frontier itself and it's a PHINode, it means that mem2reg already created a phi for this case, so we don't need to do it
	// Otherwise, if the use is dominated by the BasicBlock in-frontier, we create the vSSA_PHI
	for (Value::user_iterator begin = value->user_begin(), end = value->user_end(); begin != end; ++begin) {
		Instruction *I = cast<Instruction>(*begin);
		BasicBlock *BB_father = I->getParent();
		if (BB == BB_father && isa<PHINode>(I)) {
			continue;
		}
		if (DT_->dominates(BB, BB_father)) {
			return true;
		}
	}
	return false;
}

/*
 *  Used to insert sigma functions. Verifies if BB_next dominates any use of value (except if the use is in the same basicblock in a phi node)
 *  or if BB_next has any use of value inside its dominance frontier
 */
bool vSSA::dominateOrHasInFrontier(BasicBlock *BB, BasicBlock *BB_next, Value *value) {
	DominanceFrontier::iterator DF_BB = DF_->find(BB_next);

	for (Value::user_iterator begin = value->user_begin(), end = value->user_end(); begin != end; ++begin) {
		Instruction *I = dyn_cast<Instruction>(*begin);
		
		// ATTENTION: GEP uses are not taken into account
		if (I == NULL || isa<GetElementPtrInst>(I)) {
			continue;
		}
		
		BasicBlock *BB_father = I->getParent();
		if (BB_next == BB_father && isa<PHINode>(I))
			continue;
		
		if (DT_->dominates(BB_next, BB_father))
			return true;
		
		//If the BB_father is in the dominance frontier of BB then we need to create a sigma in BB_next 
		//to split the lifetime of variables
		if ((BB_father != BB) && (DF_BB != DF_->end()) && (DF_BB->second.find(BB_father) != DF_BB->second.end()))
			return true;
	}
	return false;
}

/*
 *  This function verifies if there is a sigma function inside BB whose incoming value is equal to V and whose incoming block is equal to BB_from
 */
bool vSSA::verifySigmaExistance(Value *V, BasicBlock *BB, BasicBlock *BB_from)
{
	for (BasicBlock::iterator it = BB->begin(); isa<PHINode>(it); ++it) {
		PHINode *sigma = cast<PHINode>(it);
		
		unsigned e = sigma->getNumIncomingValues();
		
		if (e != 1)
			continue;
			
		if ((sigma->getIncomingValue(0) == V) && (sigma->getIncomingBlock(0) == BB_from))
			return true;
	}
	
	return false;
}

char vSSA::ID = 0;
static RegisterPass<vSSA> X("vssa", "Victor's e-SSA construction", false, false);

