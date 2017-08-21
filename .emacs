;; file misc-basile/.emacs
(global-set-key (kbd "<f12>") 'recompile)
(global-set-key (kbd "<f6>") 'goto-line)
(global-set-key (kbd "<f7>") 'next-error)
(global-set-key (kbd "S-<f7>") 'previous-error)
;(global-set-key [shift f10] 'flymake-goto-next-error)
(global-auto-revert-mode t)
(global-linum-mode t)
(mouse-wheel-mode 1)
(defun linux-c-mode()
;; set gnu style.
 (c-set-style "gnu")
;; TAB offset set to 2
 (setq c-basic-offset 2)
 )
(add-hook 'c-mode-hook 'linux-c-mode)

(server-start)

(custom-set-variables
 ;; custom-set-variables was added by Custom.
 ;; If you edit it by hand, you could mess it up, so be careful.
 ;; Your init file should contain only one such instance.
 ;; If there is more than one, they won't work right.
 '(column-number-mode t)
 '(show-paren-mode t)
 '(size-indication-mode t)
 '(compile-command "time make -j 3")
 )
(custom-set-faces
 ;; custom-set-faces was added by Custom.
 ;; If you edit it by hand, you could mess it up, so be careful.
 ;; Your init file should contain only one such instance.
 ;; If there is more than one, they won't work right.
 )
