;; file .emacs
(global-set-key (kbd "<f12>") 'recompile)
(global-set-key (kbd "<f6>") 'goto-line)
(global-set-key (kbd "<f7>") 'next-error)
(global-set-key (kbd "S-<f7>") 'previous-error)
(global-auto-revert-mode t)
(global-linum-mode t)

(custom-set-variables
 ;; custom-set-variables was added by Custom.
 ;; If you edit it by hand, you could mess it up, so be careful.
 ;; Your init file should contain only one such instance.
 ;; If there is more than one, they won't work right.
 '(column-number-mode t)
 '(show-paren-mode t))
(custom-set-faces
 ;; custom-set-faces was added by Custom.
 ;; If you edit it by hand, you could mess it up, so be careful.
 ;; Your init file should contain only one such instance.
 ;; If there is more than one, they won't work right.
 )
