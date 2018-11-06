from tkinter import *

# field = Range(root, label, range1, range2)


class Range:
    def __init__(self, root, slug, label, row=0, column=0, pattern='[0-9]', cnf={}, pcnf={}, tcnf={}):

        # init
        self.root = root
        self.id = slug
        self.label_text = label
        self.row = row
        self.column = column
        self.pattern = pattern
        self.cnf = cnf
        self.pcnf = pcnf
        self.tcnf = tcnf

        # create label
        self.label = Label(self.root, text=self.label_text, **self.tcnf)
        self.label.grid(row=row, column=column)

        # create entries
        self.entries = {}
        for i in range(2):
            entry_container = Frame(self.root)
            entry = Entry(entry_container, **self.cnf)
            entry.pack(**self.pcnf)
            self.entries[self.id[i]] = entry
            entry_container.grid(row=row, column=column+i+1)

        print(self.entries)

    # returns tuple wth the entries
    def get(self):
        print(self.entries)
        return_entries = []
        for slug, entry in self.entries.items():
            return_entries.append((slug, entry.get()))

        return tuple(return_entries)

    # sets entry at index to value
    def set(self, fields):
        for field, value in fields.items():
            self.entries[field].delete(0, END)
            self.entries[field].insert(0, value)

    def validate(self):
        entry_values = {}
        value = True
        for slug, entry in self.entries.items():
            print(slug, entry)
            entry.config(background="white")
            pattern = re.compile(self.pattern)
            if not re.match(pattern, entry.get()):
                entry.config(background="red")
                value = False
            else:
                entry_values[slug] = entry.get()

        return False if value is False else entry_values


class FieldButton:
    def __init__(self, root, port, text, fields, row=0, cnf={}):
        self.port = port
        self.fields = fields
        print(cnf)
        button = Button(root, text=text, command=lambda: self.callback(), **cnf)
        button.grid(row=row)

    def callback(self):
        values = []
        for field in self.fields:
            if field.validate():
                entries = field.get()
                for entry in entries:
                    self.port.send(entry[0], entry[1])


class ActionButton:
    def __init__(self, root, port, text, commands=None, cnf={}):
        self.port = port
        self.commands = commands
        self.button = Button(root, text=text, command=lambda: self.callback(), **cnf)
        self.button.pack()

    def callback(self):
        for command, value in self.commands.items():
            self.port.send(command, value)


