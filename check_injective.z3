; ------------------------------------------------------------------------------
; LoopMix128 PRNG: Injectivity Check for Full State Transition
; ------------------------------------------------------------------------------
;
; Goal:
;   Prove that the one-step state transition function of LoopMix128 is injective.
;   The PRNG state is (fast_loop, slow_loop, mix), all uint64_t.
;
; Injectivity Criterion:
;   If two initial full states (s0,s1,m)_A and (s0,s1,m)_B are distinct,
;   then their respective next states (s0',s1',m')_A and (s0',s1',m')_B
;   must also be distinct.
;   Essentially: F(A) = F(B) implies A = B.
;
; Method:
;   This script attempts to find a counterexample. It asserts that:
;   1. (s0,s1,m)_A != (s0,s1,m)_B  (distinct initial states)
;   2. (s0',s1',m')_A = (s0',s1',m')_B (identical next states)
;
; Interpretation of Z3 Result:
;   - SAT: A counterexample exists; the transition is NOT injective. (Bad)
;   - UNSAT: No counterexample exists; the transition IS injective. (Good!)
;     An UNSAT result formally verifies that distinct full states always
;     map to distinct next full states, supporting the PRNG's period and
;     the integrity of its state evolution.
;
; Note: The PRNG's specific update rules for fast_loop, slow_loop, and mix
; (including conditional logic based on fast_loop) are modeled below.
; ------------------------------------------------------------------------------

; Set logic to Quantifier-Free Bit-Vectors
(set-logic QF_BV)

; Define the bit-vector size (matches uint64_t)
(define-sort Bv64 () (_ BitVec 64))

; Define the constant GR from your C++ code
; long GR = 0x9e3779b97f4a7c15L;
(define-const GR Bv64 #x9e3779b97f4a7c15)

; --- Declare initial states for two instances 'a' and 'b' ---
(declare-const s0_a_curr Bv64) ; Current fast_loop for instance 'a'
(declare-const s1_a_curr Bv64) ; Current slow_loop for instance 'a'
(declare-const m_a_curr Bv64)  ; Current mix for instance 'a'

(declare-const s0_b_curr Bv64) ; Current fast_loop for instance 'b'
(declare-const s1_b_curr Bv64) ; Current slow_loop for instance 'b'
(declare-const m_b_curr Bv64)  ; Current mix for instance 'b'


; --- Assert that the initial full states are distinct ---
; (s0_a_curr, s1_a_curr, m_a_curr) != (s0_b_curr, s1_b_curr, m_b_curr)
(assert (or (distinct s0_a_curr s0_b_curr)
            (distinct s1_a_curr s1_b_curr)
            (distinct m_a_curr m_b_curr)))


; --- Calculate the next state for instance 'a' ---
; (s0_a_next, s1_a_next, m_a_next)

; s0_next for 'a'
; fast_loop = rotateLeft(fast_loop, 47) + GR;
(define-const s0_a_next Bv64 (bvadd ((_ rotate_left 47) s0_a_curr) GR))

; Logic for s1 update and its effect on mix for 'a'
(define-const s1_a_cond_updated Bv64 (bvadd s1_a_curr GR)) ; s1_a_curr + GR
(define-const s1_a_for_next_cycle Bv64 (ite (= s0_a_curr (_ bv0 64)) s1_a_cond_updated s1_a_curr))

(define-const m_a_val_before_rotate Bv64
    (ite (= s0_a_curr (_ bv0 64))
         (bvxor m_a_curr s1_a_for_next_cycle) ; mix XORed with (s1_a_curr + GR if s0_curr was 0)
         m_a_curr
    )
)

; m_next for 'a'
; mix = rotateLeft(mix, 59) + fast_loop; (uses current s0_a_curr and m_a_val_before_rotate)
(define-const m_a_next Bv64 (bvadd ((_ rotate_left 59) m_a_val_before_rotate) s0_a_curr))

; s1_next for 'a'
(define-const s1_a_next Bv64 s1_a_for_next_cycle)


; --- Calculate the next state for instance 'b' ---
; (s0_b_next, s1_b_next, m_b_next)

; s0_next for 'b'
(define-const s0_b_next Bv64 (bvadd ((_ rotate_left 47) s0_b_curr) GR))

; Logic for s1 update and its effect on mix for 'b'
(define-const s1_b_cond_updated Bv64 (bvadd s1_b_curr GR))
(define-const s1_b_for_next_cycle Bv64 (ite (= s0_b_curr (_ bv0 64)) s1_b_cond_updated s1_b_curr))

(define-const m_b_val_before_rotate Bv64
    (ite (= s0_b_curr (_ bv0 64))
         (bvxor m_b_curr s1_b_for_next_cycle)
         m_b_curr
    )
)

; m_next for 'b'
(define-const m_b_next Bv64 (bvadd ((_ rotate_left 59) m_b_val_before_rotate) s0_b_curr))

; s1_next for 'b'
(define-const s1_b_next Bv64 s1_b_for_next_cycle)


; --- Assert that the resulting next full states are identical ---
(assert (and (= s0_a_next s0_b_next)
             (= s1_a_next s1_b_next)
             (= m_a_next m_b_next)))


; --- Check for satisfiability and get a model ---
; If SAT: A counterexample to injectivity is found.
; If UNSAT: The one-step state transition function is injective.
(check-sat)
(get-model)
