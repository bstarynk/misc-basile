;; file misc-basile/.emacs  -*- lexical-binding: t; -*-
;; SPDX-License-Identifier: GPL-3.0-or-later
;;
;;  © Copyright 2017 - 2025 Basile Starynkevitch (92340 Bourg-la-Reine, France)
;;   <basile@starynkevitch.net>
;;
;; 
;;
;; License: GPLv3+ (file COPYING-GPLv3)
;;    This software is free software: you can redistribute it and/or modify
;;    it under the terms of the GNU General Public License as published by
;;    the Free Software Foundation, either version 3 of the License, or
;;    (at your option) any later version.

(global-set-key (kbd "<f12>") 'recompile)
(global-set-key (kbd "<f6>") 'goto-line)
(global-set-key (kbd "<f7>") 'next-error)
(global-set-key (kbd "S-<f7>") 'previous-error)
;(global-set-key [shift f10] 'flymake-goto-next-error)
(global-auto-revert-mode t)
; (global-linum-mode t)
(mouse-wheel-mode 1)
;; BASILE: ninja-mode from github.com/ninja-build/ninja/blob/master/misc/ninja-mode.el
;; for http://ninja-build.org/ tool
(load-file "~/.emacs.d/ninja-mode.el")
(defun linux-c-mode()
;; set gnu style.
 (c-set-style "gnu")
;; TAB offset set to 2
 (setq c-basic-offset 2)
 )
;(load-file "~/.emacs.d/caml-font.el")
(add-hook 'c-mode-hook 'linux-c-mode)
(require 'package)
(add-to-list 'package-archives '("melpa" . "https://melpa.org/packages/") t)
;; Comment/uncomment this line to enable MELPA Stable if desired.  See `package-archive-priorities`
;; and `package-pinned-packages`. Most users will not need or want to do this.
;;(add-to-list 'package-archives '("melpa-stable" . "https://stable.melpa.org/packages/") t)
(add-to-list 'auto-mode-alist '("\\.ml[iylp]?$" . caml-mode))
(add-to-list 'auto-mode-alist '("\\.ocaml$" . caml-mode))
(if window-system (require 'caml-font))
(package-initialize)
(setenv "ESHELL" "/bin/zsh")

;(load-file "/usr/share/emacs/site-lisp/elpa-src/rust-mode-0.4.0/rust-mode-autoloads.el")

(server-start)

(custom-set-variables
 ;; custom-set-variables was added by Custom.
 ;; If you edit it by hand, you could mess it up, so be careful.
 ;; Your init file should contain only one such instance.
 ;; If there is more than one, they won't work right.
 '(column-number-mode t)
 '(compile-command "nice make -j4")
 '(global-display-line-numbers-mode t)
 '(package-selected-packages '(caml))
 '(size-indication-mode t))
(custom-set-faces
 ;; custom-set-faces was added by Custom.
 ;; If you edit it by hand, you could mess it up, so be careful.
 ;; Your init file should contain only one such instance.
 ;; If there is more than one, they won't work right.
 )
(put 'scroll-left 'disabled nil)
