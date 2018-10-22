from tkinter import *

class Field:
    """
    Used to create a new label + field
    """
    def __init__(self, root, t_label, fields, input_type="str", row=0, callback=None):
        """
            Create a new Field

            :param root: a Tkinter container
            :param label: The label of the field
            :param fields: list of field ids
            :param row: the row where the label + field will be placed
            :param callback: a function that triggers on change
            :type root: Tk()
            :type label: str
            :type label: tuple
            :type row: int
            :type callback: function

        """
        self.fields = fields
        self.callback = callback
        self.input_type = input_type

        label = Label(root, text=t_label)
        label.grid(column=0, row=row)

        # create StringVar() for on change events

        # source: https://stackoverflow.com/questions/6548837/how-do-i-get-an-event-callback-when-a-tkinter-entry-widget-is-modified

        self.fields_list = []
        for field_index in range(0, len(self.fields)):

            self.entry_from = Entry(root)
            self.entry_from.grid(column=field_index + 1, row=row)
            self.fields_list.append(self.entry_from)

    def mark(self, color):
        for field_index in range(0, len(self.fields)):
            self.fields_list[field_index].configure(bg=color)

    def set(self):
        pass

    def get(self):
        fields = []
        for field in self.fields_list:

            if is_type(field.get(), self.input_type):
                fields.append(field.get())
                self.mark("white")

            else:
                self.mark("red")
                break

        return fields


def is_type(value, input_type):
    # checks if value is given type
    # !!only works with int or string
    if input_type is "int":
        try:
            int(value)
        except:
            return False
        return True
    return True




# field
#