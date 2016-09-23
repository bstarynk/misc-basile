#!/usr/bin/ocaml
(*** a typical test would be
   ./basilemap.ml basilemap.ml manydl.c
 ***)
(***********************************************************************)
(*                                                                     *)
(*                                OCaml                                *)
(*                                                                     *)
(*            Xavier Leroy, projet Cristal, INRIA Rocquencourt         *)
(*                                                                     *)
(*  Copyright 1996 Institut National de Recherche en Informatique et   *)
(*  en Automatique.  All rights reserved.  This file is distributed    *)
(*  under the terms of the GNU Library General Public License, with    *)
(*  the special exception on linking described in file ../LICENSE.     *)
(*                                                                     *)
(***********************************************************************)

(*** Basile Starynkevitch is just playing to simplify slightly this
code, and to add a test case at end ***)
module type OrderedType =
  sig
    type t
    val compare: t -> t -> int
  end

module type S =
  sig
    type key
    type +'a t
    val empty: 'a t
    val is_empty: 'a t -> bool
    val mem:  key -> 'a t -> bool
    val add: key -> 'a -> 'a t -> 'a t
    val singleton: key -> 'a -> 'a t
    val remove: key -> 'a t -> 'a t
    val merge:
      (key -> 'a option -> 'b option -> 'c option) -> 'a t -> 'b t -> 'c t
    val compare: ('a -> 'a -> int) -> 'a t -> 'a t -> int
    val equal: ('a -> 'a -> bool) -> 'a t -> 'a t -> bool
    val iter: (key -> 'a -> unit) -> 'a t -> unit
    val fold: (key -> 'a -> 'b -> 'b) -> 'a t -> 'b -> 'b
    val for_all: (key -> 'a -> bool) -> 'a t -> bool
    val exists: (key -> 'a -> bool) -> 'a t -> bool
    val filter: (key -> 'a -> bool) -> 'a t -> 'a t
    val partition: (key -> 'a -> bool) -> 'a t -> 'a t * 'a t
    val cardinal: 'a t -> int
    val bindings: 'a t -> (key * 'a) list
    val min_binding: 'a t -> (key * 'a)
    val max_binding: 'a t -> (key * 'a)
    val choose: 'a t -> (key * 'a)
    val split: key -> 'a t -> 'a t * 'a option * 'a t
    val find: key -> 'a t -> 'a
    val map: ('a -> 'b) -> 'a t -> 'b t
    val mapi: (key -> 'a -> 'b) -> 'a t -> 'b t
  end

module Make(Ord: OrderedType) = struct

  type key = Ord.t

  type 'a t =
    Empty
  | Node of 'a t * key * 'a * 'a t * int

  let height n = match n with
      Empty -> 0
    | Node(_,_,_,_,h) -> h

  let create l x d r =
    let hl = height l and hr = height r in
    Node(l, x, d, r, (if hl >= hr then hl + 1 else hr + 1))

  let singleton x d = Node(Empty, x, d, Empty, 1)

  let bal l x d r =
    let hl = height l in
    let hr = height r in
    if hl > hr + 2 then begin
        match l with
          Empty -> invalid_arg "Map.bal"
        | Node(ll, lv, ld, lr, _) ->
           if height ll >= height lr then
             create ll lv ld (create lr x d r)
           else begin
               match lr with
                 Empty -> invalid_arg "Map.bal"
               | Node(lrl, lrv, lrd, lrr, _)->
                  create (create ll lv ld lrl) lrv lrd (create lrr x d r)
             end
      end else if hr > hl + 2 then begin
        match r with
          Empty -> invalid_arg "Map.bal"
        | Node(rl, rv, rd, rr, _) ->
           if height rr >= height rl then
             create (create l x d rl) rv rd rr
           else begin
               match rl with
                 Empty -> invalid_arg "Map.bal"
               | Node(rll, rlv, rld, rlr, _) ->
                  create (create l x d rll) rlv rld (create rlr rv rd rr)
             end
      end else
      create l x d r

  let empty = Empty

  let is_empty n = match n with Empty -> true | _ -> false

  let rec add x data n = match n with
      Empty ->
      create Empty x data Empty
    | Node(l, v, d, r, _) ->
       let c = Ord.compare x v in
       if c = 0 then
         create l x data r
       else if c < 0 then
         bal (add x data l) v d r
       else
         bal l v d (add x data r)

  let rec find x n = match n with
      Empty ->
      raise Not_found
    | Node(l, v, d, r, _) ->
       let c = Ord.compare x v in
       if c = 0 then d
       else find x (if c < 0 then l else r)

  let rec mem x n = match n with
      Empty ->
      false
    | Node(l, v, d, r, _) ->
       let c = Ord.compare x v in
       c = 0 || mem x (if c < 0 then l else r)

  let rec min_binding n = match n with
      Empty -> raise Not_found
    | Node(Empty, x, d, r, _) -> (x, d)
    | Node(l, x, d, r, _) -> min_binding l

  let rec max_binding n = match n with
      Empty -> raise Not_found
    | Node(l, x, d, Empty, _) -> (x, d)
    | Node(l, x, d, r, _) -> max_binding r

  let rec remove_min_binding n = match n with
      Empty -> invalid_arg "Map.remove_min_elt"
    | Node(Empty, x, d, r, _) -> r
    | Node(l, x, d, r, _) -> bal (remove_min_binding l) x d r

  let merge t1 t2 =
    match (t1, t2) with
      (Empty, t) -> t
    | (t, Empty) -> t
    | (_, _) ->
       let (x, d) = min_binding t2 in
       bal t1 x d (remove_min_binding t2)

  let rec remove x n = match n with
      Empty ->
      Empty
    | Node(l, v, d, r, h) ->
       let c = Ord.compare x v in
       if c = 0 then
         merge l r
       else if c < 0 then
         bal (remove x l) v d r
       else
         bal l v d (remove x r)

  let rec iter f n = match n with
      Empty -> ()
    | Node(l, v, d, r, _) ->
       iter f l; f v d; iter f r

  let rec map f n = match n with
      Empty ->
      Empty
    | Node(l, v, d, r, h) ->
       let l' = map f l in
       let d' = f d in
       let r' = map f r in
       Node(l', v, d', r', h)

  let rec mapi f n = match n with
      Empty ->
      Empty
    | Node(l, v, d, r, h) ->
       let l' = mapi f l in
       let d' = f v d in
       let r' = mapi f r in
       Node(l', v, d', r', h)

  let rec fold f m accu =
    match m with
      Empty -> accu
    | Node(l, v, d, r, _) ->
       fold f r (f v d (fold f l accu))

  let rec for_all p n = match n with
      Empty -> true
    | Node(l, v, d, r, _) -> p v d && for_all p l && for_all p r

  let rec exists p n = match n with
      Empty -> false
    | Node(l, v, d, r, _) -> p v d || exists p l || exists p r

  (* Beware: those two functions assume that the added k is *strictly*
       smaller (or bigger) than all the present keys in the tree; it
       does not test for equality with the current min (or max) key.

       Indeed, they are only used during the "join" operation which
       respects this precondition.
   *)

  let rec add_min_binding k v n = match n with
    | Empty -> singleton k v
    | Node (l, x, d, r, h) ->
       bal (add_min_binding k v l) x d r

  let rec add_max_binding k v n = match n with
    | Empty -> singleton k v
    | Node (l, x, d, r, h) ->
       bal l x d (add_max_binding k v r)

  (* Same as create and bal, but no assumptions are made on the
       relative heights of l and r. *)

  let rec join l v d r =
    match (l, r) with
      (Empty, _) -> add_min_binding v d r
    | (_, Empty) -> add_max_binding v d l
    | (Node(ll, lv, ld, lr, lh), Node(rl, rv, rd, rr, rh)) ->
       if lh > rh + 2 then bal ll lv ld (join lr v d r) else
         if rh > lh + 2 then bal (join l v d rl) rv rd rr else
           create l v d r

  (* Merge two trees l and r into one.
       All elements of l must precede the elements of r.
       No assumption on the heights of l and r. *)

  let concat t1 t2 =
    match (t1, t2) with
      (Empty, t) -> t
    | (t, Empty) -> t
    | (_, _) ->
       let (x, d) = min_binding t2 in
       join t1 x d (remove_min_binding t2)

  let concat_or_join t1 v d t2 =
    match d with
    | Some d -> join t1 v d t2
    | None -> concat t1 t2

  let rec split x n = match n with
      Empty ->
      (Empty, None, Empty)
    | Node(l, v, d, r, _) ->
       let c = Ord.compare x v in
       if c = 0 then (l, Some d, r)
       else if c < 0 then
         let (ll, pres, rl) = split x l in (ll, pres, join rl v d r)
       else
         let (lr, pres, rr) = split x r in (join l v d lr, pres, rr)

  let rec merge f s1 s2 =
    match (s1, s2) with
      (Empty, Empty) -> Empty
    | (Node (l1, v1, d1, r1, h1), _) when h1 >= height s2 ->
       let (l2, d2, r2) = split v1 s2 in
       concat_or_join (merge f l1 l2) v1 (f v1 (Some d1) d2) (merge f r1 r2)
    | (_, Node (l2, v2, d2, r2, h2)) ->
       let (l1, d1, r1) = split v2 s1 in
       concat_or_join (merge f l1 l2) v2 (f v2 d1 (Some d2)) (merge f r1 r2)
    | _ ->
       assert false

  let rec filter p n = match n with
      Empty -> Empty
    | Node(l, v, d, r, _) ->
       (* call [p] in the expected left-to-right order *)
       let l' = filter p l in
       let pvd = p v d in
       let r' = filter p r in
       if pvd then join l' v d r' else concat l' r'

  let rec partition p n = match n with
      Empty -> (Empty, Empty)
    | Node(l, v, d, r, _) ->
       (* call [p] in the expected left-to-right order *)
       let (lt, lf) = partition p l in
       let pvd = p v d in
       let (rt, rf) = partition p r in
       if pvd
       then (join lt v d rt, concat lf rf)
       else (concat lt rt, join lf v d rf)

  type 'a enumeration = End | More of key * 'a * 'a t * 'a enumeration

  let rec cons_enum m e =
    match m with
      Empty -> e
    | Node(l, v, d, r, _) -> cons_enum l (More(v, d, r, e))

  let compare cmp m1 m2 =
    let rec compare_aux e1 e2 =
      match (e1, e2) with
        (End, End) -> 0
      | (End, _)  -> -1
      | (_, End) -> 1
      | (More(v1, d1, r1, e1), More(v2, d2, r2, e2)) ->
         let c = Ord.compare v1 v2 in
         if c <> 0 then c else
           let c = cmp d1 d2 in
           if c <> 0 then c else
             compare_aux (cons_enum r1 e1) (cons_enum r2 e2)
    in compare_aux (cons_enum m1 End) (cons_enum m2 End)

  let equal cmp m1 m2 =
    let rec equal_aux e1 e2 =
      match (e1, e2) with
        (End, End) -> true
      | (End, _)  -> false
      | (_, End) -> false
      | (More(v1, d1, r1, e1), More(v2, d2, r2, e2)) ->
         Ord.compare v1 v2 = 0 && cmp d1 d2 &&
           equal_aux (cons_enum r1 e1) (cons_enum r2 e2)
    in equal_aux (cons_enum m1 End) (cons_enum m2 End)

  let rec cardinal n = match n with
      Empty -> 0
    | Node(l, _, _, r, _) -> cardinal l + 1 + cardinal r

  let rec bindings_aux accu n = match n with
      Empty -> accu
    | Node(l, v, d, r, _) -> bindings_aux ((v, d) :: bindings_aux accu r) l

  let bindings s =
    bindings_aux [] s

  let choose = min_binding

end
;;



(************ below test code added by Basile Starynkevitch ****************)
                                  
#load "str.cma";;

let basilemaintest () =
  let module StringMap = Make(String) in
  let
    inputname = ref "*stdin*"
  and
    inp = ref stdin
  and
    outputname = ref "*stdout*"
  and
    outp = ref stdout
  in
  let runtest () = begin
    let mapref = ref (StringMap.empty : int StringMap.t) in
    let idregex = Str.regexp "[a-zA-Z][a-zA-Z0-9_]*" in
    let processline line =
      List.iter
        (function
           Str.Delim w ->
           let cnt = try StringMap.find w !mapref with Not_found -> 0 in
           mapref := StringMap.add w (cnt+1) !mapref
         | Str.Text _ -> ())
        (Str.full_split idregex line)
    in
    let process () =
      let rec dolines count =
        let (line , more) =
          try (input_line !inp, true) with End_of_file -> ("", false)
        in
        if more then begin
            processline line;
            dolines (count+1)
          end
        else
          count
      in
      Printf.printf "#reading from %s\n" !inputname;
      flush_all ();
      let linecount = dolines 0 in
      Printf.printf "#read %d lines from %s\n" linecount !inputname;
      flush_all ()
    in
    let do_output () =
      let curmap = !mapref in
      Printf.fprintf !outp "** %d words\n" (StringMap.cardinal curmap);
      StringMap.iter (fun w c -> Printf.fprintf !outp " %s: %d\n" w c) curmap;
      flush_all ()
    in
    Arg.parse
      [
        "-in" ,
        Arg.String
          (fun argin ->
            close_in !inp;
            if argin = "-" then begin
                inp := stdin;
                inputname := "*stdin*"
              end
            else begin
                inp := open_in argin;
                inputname := argin
              end;
            process ()
          ) ,
        "set the input file"
      ;
        "-out" ,
        Arg.String
          (fun argout ->
            close_out !outp;
            if argout = "-" then begin
                outp := stdout;
                outputname := "*stdout*"
              end
            else begin
                outp := open_out argout;
                outputname := argout
              end;
            do_output ()
          ),
        "set the output file and give output"
      ;
        "-run",
        Arg.Unit do_output,
        "run and give output"
      ;
        "-reset",
        Arg.Unit (fun () -> mapref := (StringMap.empty : int StringMap.t)),
        "reset the word count map"
      ]
      begin (* the anon_fun for Arg.parse *)
        fun argstr ->
        close_in !inp;
        if argstr = "-" then
          begin
            inp := stdin;
            inputname := "*stdin*"
          end
        else
          begin
            inp := open_in argstr;
            inputname := argstr
          end;
        process ();
        do_output ()
      end
      "basilemap test"
    end (*runtest*)
  in
   runtest ()
in
if (Array.length (Sys.argv)) >= 1 then
  begin
    basilemaintest ();
    flush_all ()
  end
;;

  (** end of file basilemap.ml **)
