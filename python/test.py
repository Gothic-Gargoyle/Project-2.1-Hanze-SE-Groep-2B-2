from tkinter import *
from tkinter.ttk import *


class NotebookDemo(Frame):

    def __init__(self, isapp=True, name='notebookdemo'):
        Frame.__init__(self, name=name)
        self.pack(expand=Y, fill=BOTH)
        self.master.title('Notebook Demo')
        self.isapp = isapp
        self._create_widgets()

    def _create_widgets(self):
        self._create_demo_panel()

    def _create_demo_panel(self):
        demoPanel = Frame(self, name='demo')
        demoPanel.pack(side=TOP, fill=BOTH, expand=Y)

        # create the notebook
        nb = Notebook(demoPanel, name='notebook')

        # extend bindings to top level window allowing
        #   CTRL+TAB - cycles thru tabs
        #   SHIFT+CTRL+TAB - previous tab
        #   ALT+K - select tab using mnemonic (K = underlined letter)
        nb.enable_traversal()

        nb.pack(fill=BOTH, expand=Y, padx=2, pady=3)
        frame = Frame(demoPanel)
        self._create_descrip_tab(nb, frame, "com1")
        frame2 = Frame(demoPanel)
        self._create_descrip_tab(nb, frame2, "com2")
        frame3 = Frame(demoPanel)
        self._create_descrip_tab(nb, frame3, "com3")

        Label(frame, text="frame 1").pack()
        Label(frame2, text="frame 2").pack()
        Label(frame3, text="frame 3").pack()


    def _create_descrip_tab(self, nb, frame, name):
        # frame to hold contentx

        # add to notebook (underline = index for short-cut character)
        nb.add(frame, text=name, underline=0, padding=2)



if __name__ == '__main__':
    NotebookDemo().mainloop()
