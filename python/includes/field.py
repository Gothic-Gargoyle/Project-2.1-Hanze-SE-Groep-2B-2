from tkinter import *

# field = Range(root, label, range1, range2)


class Range:
    def __init__(self, root, slug, label, row=0, column=0, pattern='[0-9]', cnf={}, pcnf={}, tcnf={}):

        # init
        self.root = root
        self.id = slug
        self.label = label
        self.row = row
        self.column = column
        self.pattern = pattern
        self.cnf = cnf
        self.pcnf = pcnf
        self.tcnf = tcnf

        # create label
        self.label = Label(self.root, text=self.label, **self.tcnf)
        self.label.grid(row=row, column=column)

        # create entries
        self.entries = []
        for i in range(2):
            entry_container = Frame(self.root)
            entry = Entry(entry_container, **self.cnf)
            entry.pack(**self.pcnf)
            entry_container.grid(row=row, column=column+i+1)
            self.entries.append(entry)

    # returns tuple wth the entries
    def get(self):
        print(self.entries)
        return_entries = []
        for entry in self.entries:
            return_entries.append(entry.get())

        return tuple(return_entries)

    # sets entry at index to value
    def set(self, index, value):
        self.entries[index].delete(0, END)
        self.entries[index].insert(0, value)

    def validate(self):
        entry_values = []
        value = True
        for entry in self.entries:
            entry.config(background="white")
            pattern = re.compile(self.pattern)
            if not re.match(pattern, entry.get()):
                entry.config(background="red")
                value = False
            else:
                entry_values.append(entry.get())

        return False if value is False else "_".join(entry_values)

