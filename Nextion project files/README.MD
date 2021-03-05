![Algorithm schema](./images/Nex_Button.jpg)

1. Create a global variable
   
   ```c
   Nextion_Object_t buttonObject;
   ```

2. Assign component type, name, id and page id to component variable(from Nextion Editor)
   
   ```c
       buttonObject.Name = "bt0";
       buttonObject.Component_ID = 3;
       buttonObject.Page_ID = 0;
       buttonObject.dataType = OBJ_TYPE_TXT;
   ```

3. If the component needs to trigger an event, assign the function address to the appropriate callback variable, otherwise use NULL
   
   ```c
    buttonObject.PressCallback = &btnPressFnc;
    buttonObject.ReleaseCallback = NULL;
   ```

4. Finally, if you assigned a function to the component, add it to the lookup table with the following function: (If no function is assigned to a component event, skip this step, both callbacks are NULL)
   
   ```c
   NxHmi_AddObject(&buttonObject);
   ```
