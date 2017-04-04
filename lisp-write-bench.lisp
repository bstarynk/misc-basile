;; file lisp-write-bench.lisp
;; a benchmark for writing  data in SBCL

#|
(defun make-random-data (depth)
  (declare (fixnum depth)
	   (optimize (speed 3)))
  (if (zerop depth)
      (let ( (k (random 8))
	     )
	(case k
	      ((0 1 2)
	       (random 1024)
	       ;; missing code
	       )
	      (otherwise
	       ;;; missing code
	       )
	      )
	)
    (let ( (q (random 8))
	   )
      (case q
	    ;; missing code
	    )
      )
    ))
|#



